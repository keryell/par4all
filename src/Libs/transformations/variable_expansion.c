/*

  $Id$

  Copyright 1989-2009 MINES ParisTech

  This file is part of PIPS.

  PIPS is free software: you can redistribute it and/or modify it
  under the terms of the GNU General Public License as published by
  the Free Software Foundation, either version 3 of the License, or
  any later version.

  PIPS is distributed in the hope that it will be useful, but WITHOUT ANY
  WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE.

  See the GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with PIPS.  If not, see <http://www.gnu.org/licenses/>.

*/
/* transformation package :  Francois Irigoin, October 2005
 *
 * variable_expansion.c
 * ~~~~~~~~~~~~~~~~~~~~
 *
 * This File contains the functions expanding local variables used in DO loops into arrays.
 * SG: added a transformation to expand reduction variables
 */

#include <stdlib.h>
#include <stdio.h>

#include "genC.h"

#include "linear.h"
#include "ri.h"
#include "database.h"

#include "ri-util.h"
#include "pipsdbm.h"
#include "properties.h"

#include "misc.h"

#include "resources.h"
#include "reductions_private.h"
#include "reductions.h"
#include "transformations.h"
#include "control.h"
// { should not be there
#include "statistics.h"
// }

/* bool scalar_expansion(char *module_name) 
 * input    : the name of the current module
 * output   : nothing.
 * modifies : replaces local scalar variables by arrays and updates the corresponding references
 * comment  : private scalars must have been detected first (see PRIVATIZE_MODULE).
 *            array expansion would be performed in the exact same way.
 */

/* list of expressions referencing the loop indices */

typedef struct  {
    list loop_indices ;
    list loop_dimensions ;
    entity expanded_variable ;
    list processed_variables ;
} scalar_expansion_context;
#define DEFAULT_SCALAR_EXPANSION_CONTEXT { NIL,NIL,entity_undefined,NIL }

static bool perform_reference_expansion(reference r,scalar_expansion_context *ctxt)
{
  entity v = reference_variable(r);

  if(same_entity_p(v,ctxt->expanded_variable) ) {
    list il = reference_indices(r);

    pips_assert("scalar reference", ENDP(il));

    pips_debug(9, "for variable %s\n", entity_local_name(v));

    reference_indices(r) = gen_copy_seq(ctxt->loop_indices);

    ifdebug(9) {
      pips_debug(9, "New reference:");
      print_reference(r);
      pips_debug(9, "\n");
    }
  }
  return TRUE;
}

static
void
perform_reference_expansion_in_loop(instruction i,scalar_expansion_context *ctxt)
{
    if(instruction_loop_p(i))
    {
        loop l = instruction_loop(i);
        range r = loop_range(l);
        if(same_entity_p(loop_index(l),ctxt->expanded_variable) ) 
        {
            expression new_ref = make_ref_expr(ctxt->expanded_variable,gen_copy_seq(ctxt->loop_indices));
            /* we have to convert the do-loop into a for-loop */
            forloop new_loop = make_forloop(
                    expression_undefined,
                    MakeBinaryCall(
                        entity_intrinsic(LESS_OR_EQUAL_OPERATOR_NAME),
                        copy_expression(new_ref),
                        range_upper(r)),
                    MakeBinaryCall(
                        entity_intrinsic(PLUS_UPDATE_OPERATOR_NAME),
                        copy_expression(new_ref),
                        range_increment(r)),
                    loop_body(l)
                    );
            range_upper(r)=expression_undefined;
            range_lower(r)=expression_undefined;
            range_increment(r)=expression_undefined;
            loop_body(l)=statement_undefined;
            free_loop(l);
            instruction_tag(i)=is_instruction_forloop;
            instruction_forloop(i)=new_loop;
        }
    }
}

static
bool prepare_expansion(loop l, scalar_expansion_context* ctxt)
{
    entity i = loop_index(l);
    range r = loop_range(l);
    expression lb = range_lower(r);
    expression ub = range_upper(r);
    expression inc = range_increment(r);
    int ilb = 0;
    int iub = 0;
    int iinc = 0;

    /* Is this loop OK? */
    if(expression_integer_value(lb, &ilb)
            && expression_integer_value(ub, &iub)
            && expression_integer_value(inc, &iinc)
            && (iinc==1 /* || iiinc==-1 */ )) {
        dimension d = make_dimension(copy_expression(lb), copy_expression(ub));
        expression ie = entity_to_expression(i);

        /* Update information about the nesting loops. */
        ctxt->loop_dimensions = gen_append(ctxt->loop_dimensions, CONS(DIMENSION, d, NIL));
        ctxt->loop_indices = gen_append(ctxt->loop_indices, CONS(EXPRESSION, ie, NIL));

        ifdebug(9) {
            pips_debug(9, "Going down, local variables: ");
            print_entities(loop_locals(l));
            pips_debug(9, "\n");
        }
        return true;
    }

    /* If go down is FALSE here, we should reset loop_indices and
       loop_dimensions and go down anyway.*/

    return false;
}

static void perform_expansion_and_unstack_index_and_dimension(loop l,scalar_expansion_context* ctxt)
{
    entity i = loop_index(l);

    /* Select loops marked as relevant on the way down. */
    if(!ENDP(ctxt->loop_indices)) {
        expression eli = EXPRESSION(CAR(gen_last(ctxt->loop_indices)));
        if(same_entity_p(i,reference_variable(expression_reference(eli)))) {
            dimension d = DIMENSION(CAR(gen_last(ctxt->loop_dimensions)));
            list evl = NIL;

            ifdebug(9) {
                pips_debug(9, "Going up, local variables: ");
                print_entities(loop_locals(l));
                pips_debug(9, "\n");
            }

            /* Does it contain private variables? */
            FOREACH(ENTITY, lv, loop_locals(l))
            {
                /* Do not expand loop indices nor variables already processed! */
                if( !same_entity_p(lv,i) && !gen_in_list_p(lv, ctxt->processed_variables)) {
                    type t = entity_type(lv);
                    variable v = type_variable(t);
                    list dims = variable_dimensions(v);
                    statement bs = loop_body(l);

                    pips_assert("Scalar expansion", ENDP(dims) );

                    evl = CONS(ENTITY, lv, evl);

                    pips_debug(9, "Update type of %s\n", entity_local_name(lv));

                    /* Update its type */
                    variable_dimensions(v) = gen_full_copy_list(ctxt->loop_dimensions);

                    /* print_type(); */

                    /* Update its references in the loop body */

                    pips_debug(9, "Expand references to %s\n", entity_local_name(lv));
                    ctxt->expanded_variable = lv;
                    gen_context_multi_recurse(bs,ctxt, reference_domain, perform_reference_expansion, gen_null,
                            instruction_domain,gen_true,perform_reference_expansion_in_loop,
                            0);
                    ctxt->expanded_variable = entity_undefined;
                }
            }

            /* Remove the expanded variables and the loop index from the local variable list */
            gen_list_and_not(&loop_locals(l), ctxt->processed_variables);
            gen_list_and_not(&loop_locals(l), evl);
            ctxt->processed_variables = gen_append(ctxt->processed_variables, evl);
            ctxt->processed_variables = gen_append(ctxt->processed_variables, CONS(ENTITY, i, NIL));

            gen_remove(&ctxt->loop_indices, (void *) eli);
            gen_remove(&ctxt->loop_dimensions, (void *) d);
            free_dimension(d);
        }
    }
}

bool scalar_expansion(char *module_name)
{
    pips_user_warning("\nExperimental phase: on-going debugging!\n");
    /* prelude */
    set_current_module_entity(module_name_to_entity(module_name) );
    set_current_module_statement( (statement) db_get_memory_resource(DBR_CODE, module_name, TRUE) );
    debug_on("SCALAR_EXPANSION_DEBUG_LEVEL");
    pips_debug(1, "begin\n");

    /*

      Go down statements recursively.
      Each time you enter a loop:
       * if the loop bounds are constant and if the increment is one,
       stack them on the bound stack(s) together with the loop index;
       * else, stop the recursive descent.
      When new constants bounds have been found, look for local scalar variables.
      Modify the declaration of the scalar variable in the symbol table according 
      to the bound stack.
      Modify all its references in the body statement, using the stacked loop indices
      as reference.
      Remove it from the local variable field of the current DO loop.

     */
    scalar_expansion_context ctxt = DEFAULT_SCALAR_EXPANSION_CONTEXT;
    gen_context_recurse(get_current_module_statement(),&ctxt, loop_domain, prepare_expansion, perform_expansion_and_unstack_index_and_dimension);

    /* validate */
    DB_PUT_MEMORY_RESOURCE(DBR_CODE, module_name, get_current_module_statement() );

    /* postlude */
    pips_assert("Supporting lists are both empty", ENDP(ctxt.loop_indices));
    pips_assert("Supporting lists are both empty", ENDP(ctxt.loop_dimensions));

    gen_free_list(ctxt.processed_variables);

    /* Declarations must be regenerated for the code to be compilable */
    free(code_decls_text(value_code(entity_initial(get_current_module_entity()))));
    code_decls_text(value_code(entity_initial(get_current_module_entity()))) = strdup("");

    reset_current_module_entity();
    reset_current_module_statement();
    pips_debug(1, "end\n");
    debug_off();

    return true;
}

/** 
 * alias for scalar expansion
 */
bool variable_expansion(char *module_name)
{
  return scalar_expansion(module_name);
}

/** 
 * if @a r entity is the same as  @a oni[0], repalce the entity by @a oni[1] and append the index @a oni[2]
 * 
 * @param r reference to check
 * @param oni [0] old entity to be replaced by [1] with [2] additionnal index
 */
static void do_expand_reference(reference r, entity oni[3])
{
    if(same_entity_p(reference_variable(r),oni[0]))
    {
        reference_variable(r)=oni[1];
        reference_indices(r)=gen_append(reference_indices(r),CONS(EXPRESSION,entity_to_expression(oni[2]),NIL));
    }
}

/** 
 * call do_expand_reference on each entity of @a s declarations
 * 
 * @param s statement to check
 * @param oni forwarded to do_expand_reference
 */
static
void do_expand_reference_in_declarations(statement s, entity oni[3])
{
    FOREACH(ENTITY,e,statement_declarations(s))
    {
        value v = entity_initial(e);
        if( !value_undefined_p(v) && value_expression_p( v ) )
            gen_context_recurse(v,oni,reference_domain,gen_true,do_expand_reference);
    }
}


/**
 * performs scalar_expansion on variables used for reductions
 * too
 */
bool reduction_variable_expansion(char *module_name) {
    /* prelude */
    set_current_module_statement((statement)db_get_memory_resource(DBR_CODE, module_name,true));
    set_current_module_entity(module_name_to_entity(module_name));
    set_cumulated_reductions((pstatement_reductions) db_get_memory_resource(DBR_CUMULATED_REDUCTIONS, module_name, true));

    /* do the job */
    string slabel = get_string_property_or_ask("LOOP_LABEL","enter the label of a loop !");
    entity elabel = find_label_entity(module_name,slabel);

    if(entity_undefined_p(elabel))
        pips_user_error("label %s does not exist !\n", slabel);
    else {
        statement theloopstatement = find_loop_from_label(get_current_module_statement(),elabel);
        list reductions = reductions_list(load_cumulated_reductions(theloopstatement));
        /* if any reduction found */
        if(!ENDP(reductions))
        {
            /* convert the loop range to an expression */
            loop theloop = statement_loop(theloopstatement);
            expression loop_nbiters = range_to_expression(loop_range(theloop),range_to_nbiter);
            dimension thedim = make_dimension(int_to_expression(0),make_op_exp(MINUS_OPERATOR_NAME,loop_nbiters,int_to_expression(1)));

            /* used to keep track of reference <> expanded reference */
            hash_table new_entities = hash_table_make(hash_pointer,HASH_DEFAULT_SIZE);
            list trailing_statements = NIL;

            /* each reduction will be expanded into a new entity, then a loop will perform the reduction */
            FOREACH(REDUCTION,red,reductions)
            {
                entity reduction_entity = reference_variable(reduction_reference(red));
                entity new_entity = entity_undefined;
                /* check if the reduction entity has already been processed before creating the new entity */
                if(HASH_UNDEFINED_VALUE == (new_entity = hash_get(new_entities,reduction_entity)) )
                {
                    /* the new entity is expanded from the reduction entity */
                    new_entity = make_new_array_variable_with_prefix(
                            "RED",
                            get_current_module_entity(),
                            entity_basic(reduction_entity),
                            gen_append(gen_full_copy_list(variable_dimensions(type_variable(entity_type(reduction_entity)))),CONS(DIMENSION,copy_dimension(thedim),NIL))
                            );
                    AddEntityToCurrentModule(new_entity);

                    /* perform the expansion */
                    entity oni [] = { reduction_entity, new_entity, loop_index(theloop) };
                    gen_context_multi_recurse(theloop,oni,
                            reference_domain,gen_true,do_expand_reference,
                            statement_domain,gen_true,do_expand_reference_in_declarations,
                            NULL);
                    /* register the entity */
                    hash_put(new_entities,reduction_entity,new_entity);
                }

                /* create the loop that performs the reduction */
                instruction do_the_reduction = make_instruction_loop(
                        make_loop(
                            loop_index(theloop),
                            copy_range(loop_range(theloop)),
                            make_assign_statement(
                                reference_to_expression(copy_reference(reduction_reference(red))),
                                MakeBinaryCall(
                                    reduction_operator_entity(reduction_op(red)),
                                    reference_to_expression(copy_reference(reduction_reference(red))),
                                    reference_to_expression(make_reference(new_entity,CONS(EXPRESSION,entity_to_expression(loop_index(theloop)),NIL)))
                                    )
                                ),
                            entity_empty_label(),
                            make_execution_sequential(),
                            NIL)
                        );
                /* append it */
                trailing_statements=CONS(STATEMENT,instruction_to_statement(do_the_reduction),trailing_statements);
            }
            pips_assert("some entity were created",!hash_table_empty_p(new_entities));
            pips_assert("new statement were created",!ENDP(trailing_statements));
            /* create trailing statement and appedn them */
            statement all_trailining_statements = make_block_statement(trailing_statements);
            insert_statement(theloopstatement,all_trailining_statements,false);

            hash_table_free(new_entities);

            /* commit changes */
            module_reorder(get_current_module_statement());
            DB_PUT_MEMORY_RESOURCE(DBR_CODE, module_name, get_current_module_statement());
        }
    }

    /* postlude */
    reset_cumulated_reductions();
    reset_current_module_statement();
    reset_current_module_entity();
    return true;
}
