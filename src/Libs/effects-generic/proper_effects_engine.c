/* package generic effects :  Be'atrice Creusillet 5/97
 *
 * File: proper_effects_engine.c
 * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
 *
 * This File contains the generic functions necessary for the computation of 
 * all types of proper effects and proper references.
 *
 */
#include <stdio.h>
#include <string.h>

#include "genC.h"

#include "linear.h"
#include "ri.h"
#include "database.h"

#include "misc.h"
#include "ri-util.h"
#include "control.h"
#include "constants.h"
#include "misc.h"
#include "text-util.h"
#include "text.h"
#include "makefile.h"

#include "properties.h"
#include "pipsmake.h"

#include "transformer.h"
#include "semantics.h"
#include "pipsdbm.h"
#include "resources.h"

#include "effects-generic.h"

/* For debuging

static void debug_ctxt(string s, transformer t)
{
  Psysteme p;
  fprintf(stderr, "context %p at %s\n", t, s);
  if (transformer_undefined_p(t))
    fprintf(stderr, "UNDEFINED...");
  else
    {
      p = predicate_system(transformer_relation(t));
      fprintf(stderr, "%p: %d/%d\n", p, sc_nbre_egalites(p), sc_nbre_inegalites(p));
      sc_syst_debug(p);
      assert(sc_weak_consistent_p(p));
    }
}
*/

/************************************************ TO CONTRACT PROPER EFFECTS */

static bool contract_p = TRUE;

void
set_contracted_proper_effects(bool b)
{
    contract_p = b;
}

/**************************************** LOCAL STACK FOR LOOP RANGE EFFECTS */

/* Effects on loop ranges have to be added to inner statements to model 
 * control dependances (see loop filter for PUSH).
 */

DEFINE_LOCAL_STACK(current_downward_cumulated_range_effects, effects)

void proper_effects_error_handler()
{
    error_reset_effects_private_current_stmt_stack();
    error_reset_effects_private_current_context_stack();
    error_reset_current_downward_cumulated_range_effects_stack();
}

static list
cumu_range_effects()
{
      list l_cumu_range = NIL;

      if(! current_downward_cumulated_range_effects_empty_p())
      {
	  l_cumu_range =
	      effects_dup(effects_effects
			  (current_downward_cumulated_range_effects_head()));
      }
      return(l_cumu_range);
}

static void
free_cumu_range_effects()
{
    if(! current_downward_cumulated_range_effects_empty_p())
	free_effects(current_downward_cumulated_range_effects_head());
}


/************************************************************** EXPRESSSIONS */

/* list generic_proper_effects_of_range(range r, context)
 * input    : a loop range (bounds and stride) and the context.
 * output   : the corresponding list of effects.
 * modifies : nothing.
 * comment  :	
 */
list 
generic_proper_effects_of_range(range r)
{
    list le;
    expression el = range_lower(r);
    expression eu = range_upper(r);
    expression ei = range_increment(r);

    pips_debug(5, "begin\n");

    le = generic_proper_effects_of_expression(el);
    le = gen_nconc(le, generic_proper_effects_of_expression(eu));
    le = gen_nconc(le, generic_proper_effects_of_expression(ei));

    pips_debug(5, "end\n");
    return(le);
}

/* effects of a reference that is written */
/* list proper_effects_of_lhs(reference ref)
 * input    : a reference that is written.
 * output   : the corresponding list of effects.
 * modifies : nothing.
 * comment  :	
 */
list 
generic_proper_effects_of_lhs(reference ref)
{    
    list le = NIL;
    list inds = reference_indices(ref);
    transformer context = effects_private_current_context_head();


    pips_debug(3, "begin\n");

    if (! (*empty_context_test)(context))
    {	
	le = CONS(EFFECT, 
		  (*reference_to_effect_func)(ref,
					      make_action(is_action_write, UU)),
		  NIL);

	if (! ENDP(inds)) 
	    le = gen_nconc(le, generic_proper_effects_of_expressions(inds));

	(*effects_precondition_composition_op)(le, context);
    } 

  
    pips_debug(3, "end\n");
    return(le);
}

/* list generic_proper_effects_of_reference(reference ref)
 * input    : a reference that is read.
 * output   : the corresponding list of effects.
 * modifies : nothing.
 * comment  :	
 */
list 
generic_proper_effects_of_reference(reference ref)
{
    list inds = reference_indices(ref);
    list le = NIL;
    transformer context;

    /* CA: lazy, because in the in region backward translation of formal
     * parameters that are not simple references on the caller side,
     * this stuff may be called without proper context.
     */
    if (effects_private_current_context_empty_p())
	context = transformer_undefined;
    else
      {
	context = effects_private_current_context_head();
      }


    pips_debug(3, "begin\n");
    
    if (! (*empty_context_test)(context))
    {	
	le = CONS(EFFECT, 
		  (*reference_to_effect_func)(ref,
					      make_action(is_action_read, UU)),
		  NIL);

	if (! ENDP(inds)) 
	    le = gen_nconc(le, generic_proper_effects_of_expressions(inds));


	(*effects_precondition_composition_op)(le, context);
    }

    pips_debug(3, "end\n");
    return(le);
}

/* list generic_proper_effects_of_syntax(syntax s)
 * input    : 
 * output   : 
 * modifies : 
 * comment  :	
 */
list 
generic_proper_effects_of_syntax(syntax s)
{
    list le = NIL;

    pips_debug(5, "begin\n");

    switch(syntax_tag(s))
    {
    case is_syntax_reference:
        le = generic_proper_effects_of_reference(syntax_reference(s));
        break;
    case is_syntax_range:
        le = generic_proper_effects_of_range(syntax_range(s));
        break;
    case is_syntax_call:
        le = generic_r_proper_effects_of_call(syntax_call(s));
        break;
    default:
        pips_internal_error("unexpected tag %d\n", syntax_tag(s));
    }

    ifdebug(8)
    {
	pips_debug(8, "Proper effects of expression  %s :\n",
		   words_to_string(words_syntax(s)));
	(*effects_prettyprint_func)(le);
    }

    pips_debug(5, "end\n");
    return(le);
}

/* list proper_effects_of_expression(expression e)
 * inputgeneric_    : an expression and the current context
 * output   : the correpsonding list of effects.
 * modifies : nothing.
 * comment  :	
 */
list 
generic_proper_effects_of_expression(expression e)
{
  list le = generic_proper_effects_of_syntax(expression_syntax(e));

  /* keep track of proper effects associated to sub-expressions if required.
   */
  if (!expr_prw_effects_undefined_p())
  {
    /* in IO lists, the effects are computed twice, 
     * once as LHS, once as a REFERENCE...
     * so something may already be in. Let's skip it.
     * I should investigate further maybe. FC.
     */
    if (!bound_expr_prw_effects_p(e))
      store_expr_prw_effects(e, make_effects(gen_full_copy_list(le)));
  }

  return le;
}

/* list generic_proper_effects_of_expressions(list exprs)
 * input    : a list of expressions and the current context.
 * outpproper_ut   : the correpsonding list of effects.
 * modifies : nothing.
 * comment  :	
 */
list 
generic_proper_effects_of_expressions(list exprs)
{
    list le = NIL;

    pips_debug(5, "begin\n");

    MAP(EXPRESSION, exp,
    {
	le = gen_nconc(le, generic_proper_effects_of_expression(exp));
    },
	exprs);

    pips_debug(5, "end\n");
    return(le);
}


static list 
generic_proper_effects_of_external(entity func, list args)
{
    list le = NIL;
    char *func_name = module_local_name(func);

    pips_debug(4, "translating effects for %s\n", func_name);

    if (! entity_module_p(func)) 
    {
	pips_error("proper_effects_of_external", 
		   "%s: bad function\n", func_name);
    }
    else 
    {
	list func_eff;
	transformer context;

        /* Get the in summary effects of "func". */	
	func_eff = (*db_get_summary_rw_effects_func)(func_name);
	/* Translate them using context information. */
	context = effects_private_current_context_head();
	le = (*effects_backward_translation_op)(func, args, func_eff, context);
    }
    return le;  
}

/* list proper_effects_of_call(call c, transformer context, list *plpropreg)
 * input    : a call, which can be a call to a subroutine, but also
 *            to an function, or to an intrinsic, or even an assignement.
 *            And a pointer that will be the proper effects of the call; NIL,
 *            except for an intrinsic (assignment or real FORTRAN intrinsic).
 * output   : the corresponding list of effects.
 * modifies : nothing.
 * comment  :	
 */
list 
generic_r_proper_effects_of_call(call c)
{
    list le = NIL;
    entity e = call_function(c);
    tag t = value_tag(entity_initial(e));
    string n = module_local_name(e);
    list pc = call_arguments(c);

    pips_debug(2, "begin for %s\n", entity_local_name(e));

    switch (t)
    {
    case is_value_code:
        pips_debug(5, "external function %s\n", n);
        le = generic_proper_effects_of_external(e, pc);
        break;

    case is_value_intrinsic:
        pips_debug(5, "intrinsic function %s\n", n);
        le = generic_proper_effects_of_intrinsic(e, pc);
        break;

    case is_value_symbolic:
	pips_debug(5, "symbolic\n");
	break;

    case is_value_constant:
	pips_debug(5, "constant\n");
        break;

    case is_value_unknown:
	if (get_bool_property("HPFC_FILTER_CALLEES"))
	    /* hpfc specials are managed here... */
	    le = NIL;
	else
	    pips_internal_error("unknown function %s\n", entity_name(e));
        break;

    default:
        pips_internal_error("unknown tag %d\n", t);
    }

    pips_debug(2, "end\n");

    return(le);
}


/**************************************************************** STATEMENTS */

static void 
proper_effects_of_call(call c)
{
    list l_proper=NIL;
    statement current_stat = effects_private_current_stmt_head();
    instruction inst = statement_instruction(current_stat);
    list l_cumu_range = cumu_range_effects();

    /* Is the call an instruction, or a sub-expression? */
    if (instruction_call_p(inst) && (instruction_call(inst) == c))
    {
	pips_debug(2, "Effects for statement%03d:\n",
		   statement_ordering(current_stat)); 
	l_proper = generic_r_proper_effects_of_call(c);
	l_proper = gen_nconc(l_proper, effects_dup(l_cumu_range));
		
	if (contract_p)
	    l_proper = proper_effects_contract(l_proper);
	ifdebug(2)
	    {
		pips_debug(2, "Proper effects for statement%03d:\n",
			   statement_ordering(current_stat));  
		(*effects_prettyprint_func)(l_proper);
		pips_debug(2, "end\n");
	    }

	store_proper_rw_effects_list(current_stat,l_proper);
    }
}

static void 
proper_effects_of_unstructured(unstructured u)
{
    statement current_stat = effects_private_current_stmt_head();
    store_proper_rw_effects_list(current_stat,NIL);
}

static bool
loop_filter(loop l)
{
    list l_proper = generic_proper_effects_of_range(loop_range(l));
    list l_eff = cumu_range_effects();
    
    l_eff = gen_nconc(l_eff, l_proper);
    current_downward_cumulated_range_effects_push(make_effects(l_eff));
    return(TRUE);
}

static void proper_effects_of_loop(loop l)
{
    statement current_stat = effects_private_current_stmt_head();
    list l_proper = NIL;
    list l_cumu_range = NIL;

    entity i = loop_index(l);
    range r = loop_range(l);

    list li = NIL, lb = NIL;

    pips_debug(2, "Effects for statement%03d:\n",
	       statement_ordering(current_stat)); 

    free_cumu_range_effects();
    current_downward_cumulated_range_effects_pop();
    l_cumu_range = cumu_range_effects();
    
    /* proper_effects first */

    /* Effects of loop on loop index.
     * loop index is must-written but may-read because the loop might
     * execute no iterations.
     */
    /* FI, RK: the may-read effect on the index variable is masked by
     * the initial unconditional write on it (see standard page 11-7, 11.10.3);
     * if masking is not performed, the read may prevent privatization
     * somewhere else in the module (12 March 1993)
     */
    /* Parallel case
     *
     * as I need the same effects on a parallel loop to remove
     * unused private variable in rice/codegen.c, I put the
     * same code to compute parallel loop proper effects.
     * this may not be correct, but I should be the only one to use
     * such a feature. FC, 23/09/93
     */
    li = generic_proper_effects_of_lhs(make_reference(i, NIL));

    /* effects of loop bound expressions. */
    lb = generic_proper_effects_of_range(r);

    l_proper = gen_nconc(li, lb);
    l_proper = gen_nconc(l_proper, effects_dup(l_cumu_range));
  
    ifdebug(2)
    {
	pips_debug(2, "Proper effects for statement%03d:\n",
		   statement_ordering(current_stat));  
	(*effects_prettyprint_func)(l_proper);
	pips_debug(2, "end\n");
    }

    if (contract_p)
	l_proper = proper_effects_contract(l_proper);

    store_proper_rw_effects_list(current_stat, l_proper);
}

static void proper_effects_of_while(whileloop w)
{
    statement current_stat = effects_private_current_stmt_head();
    list /* of effect */ l_proper = 
	generic_proper_effects_of_expression(whileloop_condition(w));
    store_proper_rw_effects_list(current_stat, l_proper);
}

static void proper_effects_of_test(test t)
{
    list l_proper=NIL;
    statement current_stat = effects_private_current_stmt_head();
    list l_cumu_range = cumu_range_effects();

    pips_debug(2, "Effects for statement%03d:\n",
	       statement_ordering(current_stat)); 

    /* effects of the condition */
    l_proper = generic_proper_effects_of_expression(test_condition(t));
    l_proper = gen_nconc(l_proper, effects_dup(l_cumu_range));
    
    ifdebug(2)
    {
	pips_debug(2, "Proper effects for statement%03d:\n",
		   statement_ordering(current_stat));  
	(*effects_prettyprint_func)(l_proper);
	pips_debug(2, "end\n");
    }

    if (contract_p)
	l_proper = proper_effects_contract(l_proper);
    store_proper_rw_effects_list(current_stat, l_proper);
}

static void proper_effects_of_sequence(sequence block)
{
    statement current_stat = effects_private_current_stmt_head();   
    store_proper_rw_effects_list(current_stat, NIL);
}

static bool stmt_filter(statement s)
{
  pips_debug(1, "Entering statement %03d :\n", statement_ordering(s));
  effects_private_current_stmt_push(s);
  effects_private_current_context_push((*load_context_func)(s));
  return(TRUE);
}

static void proper_effects_of_statement(statement s)
{
    if (!bound_proper_rw_effects_p(s)) 
     { 
 	pips_debug(2, "Warning, proper effects undefined, set to NIL"); 
 	store_proper_rw_effects_list(s,NIL);	 
     } 
    effects_private_current_stmt_pop();
    effects_private_current_context_pop();

    pips_debug(1, "End statement%03d :\n", statement_ordering(s));
  
}

void proper_effects_of_module_statement(statement module_stat)
{    
    make_effects_private_current_stmt_stack();
    make_effects_private_current_context_stack();
    make_current_downward_cumulated_range_effects_stack();
    pips_debug(1,"begin\n");
    
    gen_multi_recurse
	(module_stat, 
	 statement_domain, stmt_filter, proper_effects_of_statement,
	 sequence_domain, gen_true, proper_effects_of_sequence,
	 test_domain, gen_true, proper_effects_of_test,
	 call_domain, gen_true, proper_effects_of_call,
	 loop_domain, loop_filter, proper_effects_of_loop,
	 whileloop_domain, gen_true, proper_effects_of_while,
	 unstructured_domain, gen_true, proper_effects_of_unstructured,
	 expression_domain, gen_false, gen_null, /* NOT THESE CALLS */
	 NULL); 

    pips_debug(1,"end\n");
    free_effects_private_current_stmt_stack();
    free_effects_private_current_context_stack();
    free_current_downward_cumulated_range_effects_stack();
}

bool proper_effects_engine(char *module_name)
{    
    /* Get the code of the module. */
    set_current_module_statement( (statement)
		      db_get_memory_resource(DBR_CODE, module_name, TRUE) );
    
    set_current_module_entity( local_name_to_top_level_entity(module_name) );

    (*effects_computation_init_func)(module_name);

    /* Compute the effects or references of the module. */
    init_proper_rw_effects();
  
    debug_on("PROPER_EFFECTS_DEBUG_LEVEL");
    pips_debug(1, "begin\n");

    proper_effects_of_module_statement(get_current_module_statement()); 

    pips_debug(1, "end\n");
    debug_off();

    (*db_put_proper_rw_effects_func)(module_name, get_proper_rw_effects());

    reset_current_module_entity();
    reset_current_module_statement();
    reset_proper_rw_effects();

    (*effects_computation_reset_func)(module_name);
    
    return(TRUE);
}

/* compute proper effects for both expressions and statements
   WARNING: the functions are set as a side effect.
 */
void 
expression_proper_effects_engine(
    string module_name,
    statement current)
{    
    (*effects_computation_init_func)(module_name);

    init_proper_rw_effects();
    init_expr_prw_effects();
  
    debug_on("PROPER_EFFECTS_DEBUG_LEVEL");
    pips_debug(1, "begin\n");

    proper_effects_of_module_statement(current); 

    pips_debug(1, "end\n");
    debug_off();

    (*effects_computation_reset_func)(module_name);
}
