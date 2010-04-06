/*

  $Id$

  Copyright 1989-2010 MINES ParisTech

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
#ifdef HAVE_CONFIG_H
    #include "pips_config.h"
#endif
/* Created by B. Apvrille, april 11th, 1994 */
/* functions related to types effects and effect, cell, reference and
   gap */

#include <stdio.h>

#include "linear.h"

#include "genC.h"

#include "ri.h"

/* #include"mapping.h" */
#include "misc.h"

#include "ri-util.h"

/* API for reference */

/* Does the set of locations referenced by r depend on a pointer
   dereferencing?

   Let's hope that all Fortran 77 references will return false...
*/
bool memory_dereferencing_p(reference r)
{
  bool dereferencing_p = FALSE;
  entity v = reference_variable(r);
  type vt = entity_type(v);
  type uvt = ultimate_type(vt);
  list sl = reference_indices(r); // subscript list

  /* Get rid of simple Fortran-like array accesses */
  if(gen_length(sl)==gen_length(variable_dimensions(type_variable(vt)))) {
    /* This is a simple array access */
    dereferencing_p = FALSE;
  }
  else if(!ENDP(sl)) {
    /* cycle with alias-classes library: import explictly */
    bool entity_abstract_location_p(entity);
    if(entity_abstract_location_p(v)) {
      pips_internal_error("Do we want to subscript abstract locations?\n");
    }
    else if(FALSE /* entity_heap_variable_p(v)*/) {
      /* Heap modelization is behind*/
    }
    else if(entity_variable_p(v)) {
    /* Let's walk the subscript list and see if the type associated
       to the nth subscript is a pointer type and if the (n+1)th
       subscript is a zero. */
      if(pointer_type_p(uvt))
	/* Since it is subscripted, there is dereferencing */
	dereferencing_p = TRUE;
      else {
	list csl = sl; // current subscript list
	type ct = uvt; // current type
	while(!dereferencing_p && !ENDP(csl)) {
	  expression se = EXPRESSION(CAR(csl));
	  ct = ultimate_type(subscripted_type_to_type(ct, se));
	  if(pointer_type_p(ct) && !ENDP(csl)) {
	    dereferencing_p = TRUE;
	  }
	  else {
	    POP(csl);
	  }
	}
      }
    }
    else {
      pips_internal_error("Unexpected entity kind \"%s\"", entity_name(v));
    }
  }
  else {
    /* No dereferencing */
    ;
  }

  return dereferencing_p;
}

/* May the two references to array a using subscript list sl1 and sl2 access
   the same memory locations?

   Subscript list sl1 and sl2 can be evaluated in two different
   stores.

   It is assumed that dim(a)=length(sl1)=length(sl2);

   If the nth subscript expression can be statically evaluated in both sl1
   and sl2 and if the subscript values are different, there is not
   conflict. For instance a[i][0] does not conflict with a[j][1].

   This is about the old references_conflict_p()
*/
bool array_references_may_conflict_p(list sl1, list sl2)
{
  bool conflict_p = TRUE;

  list cind1 = list_undefined;
  list cind2 = list_undefined;
  for(cind1 = sl1, cind2 = sl2;
      !ENDP(cind1) && !ENDP(cind2);
      POP(cind1), POP(cind2)) {
    expression e1 = EXPRESSION(CAR(cind1));
    expression e2 = EXPRESSION(CAR(cind2));
    if(unbounded_expression_p(e1)||unbounded_expression_p(e2))
      conflict_p = TRUE;
    else {
      int i1 = -1;
      int i2 = -1;
      bool i1_p = FALSE;
      bool i2_p = FALSE;

      i1_p = expression_integer_value(e1, &i1);
      i2_p = expression_integer_value(e2, &i2);
      if(i1_p && i2_p)
	conflict_p = (i1==i2);
      else
	conflict_p = TRUE;
    }
  }
  return conflict_p;
}

/* May the two references to v using subscript list sl1 and sl2 access
   the same memory locations?

   Subscript list sl1 and sl2 can be evaluated in two different stores.
*/
bool variable_references_may_conflict_p(entity v, list sl1, list sl2)
{
  bool conflict_p = TRUE;
  type t = entity_type(v);
  int sl1n = gen_length(sl1);
  int sl2n = gen_length(sl2);
  int nd = gen_length(variable_dimensions(type_variable(t)));

  if(sl1n==sl2n && sl1n==nd) {
    /* This is equivalent to a simple Fortran-like array reference */
    conflict_p = array_references_may_conflict_p(sl1, sl2);
  }
  else {
    if(!ENDP(sl1) && !ENDP(sl2)) {
      list cind1 = list_undefined;
      list cind2 = list_undefined;
      /* FI: this is new not really designed (!) code */
      for(cind1 = sl1, cind2 = sl2; !ENDP(cind1) && !ENDP(cind2) && conflict_p; POP(cind1), POP(cind2)) {
	expression e1 = EXPRESSION(CAR(cind1));
	expression e2 = EXPRESSION(CAR(cind2));
	if(unbounded_expression_p(e1)||unbounded_expression_p(e2))
	  conflict_p = TRUE;
	else if(expression_reference_p(e1) && expression_reference_p(e2)) {
	  /* Because of heap modelization functions can be used as
	     subscript. Because of struct and union modelization,
	     fields can be used as subscripts. */
	  entity s1 = expression_variable(e1); // first subscript
	  entity s2 = expression_variable(e2); // second subscript
	  type s1t = entity_type(s1);
	  type s2t = entity_type(s2);

	  if(type_equal_p(s1t, s2t)) {
	    if(type_functional_p(s1t)) {
	      /* context sensitive heap modelization */
	      conflict_p = same_string_p(entity_name(s1), entity_name(s2));
	    }
	  }
	  else {
	    /* assume the code is correct... Assume no floating
	       point index... a[i] vs a[x]... */
	    conflict_p = FALSE;
	  }
	}
	else {
	  int i1 = -1;
	  int i2 = -1;
	  bool i1_p = FALSE;
	  bool i2_p = FALSE;

	  i1_p = expression_integer_value(e1, &i1);
	  i2_p = expression_integer_value(e2, &i2);
	  if(i1_p && i2_p)
	    conflict_p = (i1==i2);
	  else
	    conflict_p = TRUE;
	}
      }
    }
    else
      conflict_p = TRUE;
  }
  return conflict_p;
}

/* Can the two references r1 and r2 access the same memory location
   when evaluated in two different stores?

   We have to deal with static aliasing for Fortran and with dynamic
   aliasing for C and Fortran95.

   We have to deal with abstract locations.

   A PIPS reference is a memory access path rather than a reference as
   understood in programming languages:

   - a field of a struct or a union d can be accessed by subscripting
     d with a field number or with a field entity

   - a dereferencing is expressed by a zero subscript: *p is p[0]

   - abstract locations such as foo:*HEAP* or foo:*HEAP**MEMORY* or
     foo:*HEAP*_v3 can be used

   - heap modelization uses the malloc statement number as a subscript

   - flow-sensitive heap modelization can use indices from the
     surrounding loops as subscripts

   - context-sensitive heap modelization can also use function
     reference to record the call path

   Two boolean properties are involved:

   - ALIASING_ACROSS_TYPES: if false objects of different types cannot
     be aliased

   - ALIASING_INSIDE_DATA_STRUCTURE: if false, access paths starting
     from the same data structure are assumed disjoint. This property
     holds even after pointer dereferencement. It is extremely strong
     and wrong for PIPS source code, unless persistant is taken into
     account.

*/
bool references_may_conflict_p(reference r1, reference r2)
{
  bool conflict_p = TRUE;
  entity v1 = reference_variable(r1);
  entity v2 = reference_variable(r2);

  if(entity_conflict_p(v1, v2)) {
    list ind1 = reference_indices(r1);
    list ind2 = reference_indices(r2);

    if(v1!=v2) {
      /* We do not bother with the offset and the array types in case
	 of static aliasing */
      /* We do not bother with the abstract locations */
      conflict_p = TRUE;
    }
    else {
      /* v1 == v2 */
      conflict_p = variable_references_may_conflict_p(v1, ind1, ind2);
    }
  }
  else {
    /* Can we have some dynamic aliasing? */
    /* Do we have aliasing between types? */
    /* Do we have aliasing within a data structure? */
    bool get_bool_property(string);
    if(!get_bool_property("ALIASING_ACROSS_TYPES")) {
      type t1 = reference_to_type(r1);
      type t2 = reference_to_type(r2);

      conflict_p  = !type_equal_p(t1, t2);
    }
    if(conflict_p) {
    /* Do we have some dereferencing in ind1 or ind2? Do we assume
       that p[0] conflicts with any reference? We might as well use
       reference_to_abstract_location()... */
      /* Could be improved with ALIASING_ACROSS_DATA_STRUCTURES? */
      conflict_p = memory_dereferencing_p(r1) || memory_dereferencing_p(r2);
    }
  }
  return conflict_p;
}

bool references_must_conflict_p(reference r1 __attribute__ ((__unused__)),
				reference r2 __attribute__ ((__unused__)))
{
  bool conflict_p = FALSE;
  pips_user_warning("Not implemented yet. "
		    "Conservative under approximation is made\n");
  return conflict_p;
}

/* API for cell */
bool cells_maymust_conflict_p(cell c1, cell c2, bool must_p)
{
  bool conflict_p = FALSE;
  reference r1 = reference_undefined;
  reference r2 = reference_undefined;

  if(cell_reference_p(c1))
    r1 = cell_reference(c1);
  else if(cell_preference_p(c1))
    r1 = cell_preference(c1);

  if(cell_reference_p(c2))
    r2 = cell_reference(c2);
  else if(cell_preference_p(c2))
    r2 = cell_preference(c2);

  if(reference_undefined_p(r1) || reference_undefined_p(r2)) {
    pips_internal_error("either undefined references or gap "
			"not implemented yet\n");
  }

  conflict_p = must_p ? references_must_conflict_p(r1, r2):
    references_may_conflict_p(r1, r2);

  return conflict_p;
}

bool cells_may_conflict_p(cell c1, cell c2)
{
  bool conflict_p = cells_maymust_conflict_p(c1, c2, FALSE);
  return conflict_p;
}

bool cells_must_conflict_p(cell c1, cell c2)
{
  bool conflict_p = cells_maymust_conflict_p(c1, c2, TRUE);
  return conflict_p;
}

/* Future API for GAP, Generic Access Path*/

/* ---------------------------------------------------------------------- */
/* list-effects conversion functions                                      */
/* ---------------------------------------------------------------------- */

effects list_to_effects(l_eff)
list l_eff;
{
    effects res = make_effects(l_eff);
    return res;
}

list effects_to_list(efs)
effects efs;
{
    list l_res = effects_effects(efs);
    return l_res;
}

statement_mapping listmap_to_effectsmap(l_map)
statement_mapping l_map;
{
    statement_mapping efs_map = MAKE_STATEMENT_MAPPING();
    
    STATEMENT_MAPPING_MAP(s,val,{
	hash_put((hash_table) efs_map, (char *) s, (char *) list_to_effects((list) val));
    }, l_map);

    return efs_map;
}

statement_mapping effectsmap_to_listmap(efs_map)
statement_mapping efs_map;
{
    statement_mapping l_map = MAKE_STATEMENT_MAPPING();

    STATEMENT_MAPPING_MAP(s,val,{
	hash_put((hash_table) l_map, (char *) s, (char *) effects_to_list((effects) val));
    }, efs_map);

    return l_map;
}



/* Return TRUE if the statement has a write effect on at least one of
   the argument (formal parameter) of the module. Note that the return
   variable of a function is also considered here as a formal
   parameter. */
bool
statement_has_a_module_formal_argument_write_effect_p(statement s,
                                                      entity module,
                                                      statement_mapping effects_list_map)
{
   bool write_effect_on_a_module_argument_found = FALSE;
   list effects_list = (list) GET_STATEMENT_MAPPING(effects_list_map, s);

   MAP(EFFECT, an_effect,
       {
          entity a_variable = reference_variable(effect_any_reference(an_effect));
          
          if (action_write_p(effect_action(an_effect))
              && (variable_return_p(a_variable)
		  || variable_is_a_module_formal_parameter_p(a_variable,
							     module))) {
	      write_effect_on_a_module_argument_found = TRUE;
             break;
          }
       },
       effects_list);

   return write_effect_on_a_module_argument_found;

}

/* Anywhere effect: an effect which can be related to any location of any areas */

/* Allocate a new anywhere effect, and the anywhere entity on demand
   which may not be best if we want to express it's aliasing with all
   module areas. In the later case, the anywhere entity should be
   generated by bootstrap and be updated each time new areas are
   declared by the parsers. I do not use a persistant anywhere
   reference to avoid trouble with convex-effect nypassing of the
   persistant pointer.

   Action a is integrated in the new effect (aliasing).
   NOT GENERIC AT ALL. USE make_anywhere_effect INSTEAD (BC).
 */
effect anywhere_effect(action ac)
{
  entity anywhere = entity_all_locations();
  effect any = effect_undefined;

  any = make_effect(make_cell_reference(make_reference(anywhere, NIL)),
		    ac,
		    make_approximation_may(),
		    make_descriptor_none());

  return any;
}

/* Is it an anywhere effect? */
bool anywhere_effect_p(effect e)
{
  bool anywhere_p;
  reference r = effect_any_reference(e);
  entity v = reference_variable(r);

  anywhere_p =  entity_all_locations_p(v);

  return anywhere_p;
}

effect heap_effect(entity m, action ac)
{
  entity heap = global_name_to_entity(entity_local_name(m), HEAP_AREA_LOCAL_NAME );
  effect any = effect_undefined;

  if(entity_undefined_p(heap)) {
    pips_internal_error("Heap for module \"%s\" not found\n", entity_name(m));
  }

  any = make_effect(make_cell_reference(make_reference(heap, NIL)),
		    ac,
		    make_approximation_may(),
		    make_descriptor_none());

  return any;
}

bool heap_effect_p(effect e)
{
  bool heap_p;
  reference r = effect_any_reference(e);
  entity v = reference_variable(r);

  heap_p = same_string_p(entity_local_name(v), HEAP_AREA_LOCAL_NAME);

  return heap_p;
}

bool malloc_effect_p(effect e)
{
  bool heap_p;
  reference r = effect_any_reference(e);
  entity v = reference_variable(r);

  heap_p = same_string_p(entity_local_name(v), MALLOC_EFFECTS_NAME);

  return heap_p;
}

/*************** I/O EFFECTS *****************/
bool io_effect_entity_p(entity e)
{
    return io_entity_p(e) && 
	same_string_p(entity_local_name(e), IO_EFFECTS_ARRAY_NAME);
}

bool io_effect_p(effect e)
{
  return io_effect_entity_p(reference_variable(effect_any_reference(e)));
}

/* Can we merge these two effects because they are equal or because
   they only differ by their approximations and their descriptors? */
bool effect_comparable_p(effect e1, effect e2)
{
  bool comparable_p = FALSE;
  reference r1 = effect_any_reference(e1);
  reference r2 = effect_any_reference(e2);
  entity v1 = reference_variable(r1);
  entity v2 = reference_variable(r2);

  if(v1==v2) {
    action a1 = effect_action(e1);
    action a2 = effect_action(e2);
    if(action_tag(a1)==action_tag(a2))
      {

	/* Check the subscript lists because p and p[0] do not refer
	   the same memory locations at all */
	list sl1 = reference_indices(r1);
	list sl2 = reference_indices(r2);
	if(gen_length(sl1)==gen_length(sl2))
	  {
	    list csl1 = list_undefined;
	    list csl2 = list_undefined;
	    bool equal_p = TRUE;

	    for(csl1=sl1, csl2=sl2; !ENDP(csl1) && equal_p; POP(csl1), POP(csl2))
	      {
		expression e1 = EXPRESSION(CAR(csl1));
		expression e2 = EXPRESSION(CAR(csl2));
		equal_p = expression_equal_p(e1, e2);
	      }
	    comparable_p = equal_p;
	  }

      }
  }

  return comparable_p;
}

/* Two effects conflict if they abstract two location sets with a
   non-empty intersection and if at least one of them is a write.

   This function is conservative: it is always correct to declare a
   conflict.
 */
bool effects_may_conflict_p(effect eff1, effect eff2)
{
  action ac1 = effect_action(eff1);
  action ac2 = effect_action(eff2);
  bool conflict_p = FALSE;

  if(action_write_p(ac1)||action_write_p(ac2)) {
  }
  return conflict_p;
}
/* Two effects conflict is they abstract two location sets with a
   non-empty intersection.

   This function is conservative: it is always correct to declare a
   conflict.
 */
static bool old_effects_conflict_p(effect eff1, effect eff2)
{
  action ac1 = effect_action(eff1);
  action ac2 = effect_action(eff2);
  bool conflict_p = FALSE;

  if(action_write_p(ac1)||action_write_p(ac2)) {
    if(anywhere_effect_p(eff1)||anywhere_effect_p(eff2))
      conflict_p = TRUE;
    else {

	reference r1 = effect_any_reference(eff1);
	entity v1 = reference_variable(r1);
	reference r2 = effect_any_reference(eff2);
	entity v2 = reference_variable(r2);

	/* Do we point to the same area? */

	if(entity_conflict_p(v1, v2)) {
	  list ind1 = reference_indices(r1);
	  list ind2 = reference_indices(r2);
	  list cind1 = list_undefined;
	  list cind2 = list_undefined;

	  if(v1!=v2) {
	    /* We do not bother with the offset and the array types */
	    conflict_p = TRUE;
	  }
	  else {
	    if(!ENDP(ind1) && !ENDP(ind2)) {
	      for(cind1 = ind1, cind2 = ind2; !ENDP(cind1) && !ENDP(cind2); POP(cind1), POP(cind2)) {
		expression e1 = EXPRESSION(CAR(cind1));
		expression e2 = EXPRESSION(CAR(cind2));
		if(unbounded_expression_p(e1)||unbounded_expression_p(e2))
		  conflict_p = TRUE;
		else {
		  int i1 = -1;
		  int i2 = -1;
		  bool i1_p = FALSE;
		  bool i2_p = FALSE;

		  i1_p = expression_integer_value(e1, &i1);
		  i2_p = expression_integer_value(e2, &i2);
		  if(i1_p && i2_p)
		    conflict_p = (i1==i2);
		  else
		    conflict_p = TRUE;
		}
	      }
	    }
	    else
	      conflict_p = TRUE;
	  }
	}

      else
	conflict_p = TRUE;
    }
  }
  return conflict_p;
}

/* Synonym for effects_may_conflict_p(). name preserved to limit
   rewriting of source code using the old version. Also checks
   results of new implementation wrt the old implementatation */
bool effects_conflict_p(effect eff1, effect eff2)
{
  bool conflict_p = effects_may_conflict_p(eff1, eff2);
  bool old_conflict_p = old_effects_conflict_p(eff1, eff2);

  /* In general, there is no reason to have the same results... This
     is only a first debugging step. */
  if(conflict_p!=old_conflict_p)
    pips_internal_error("Inconsistant conflict detection.\n");

  return conflict_p;
}

/* Does this effect define the same set of memory locations
   regardless of the current (environment and) memory state?
 */
bool store_independent_effect_p(effect eff)
{
  bool independent_p = FALSE;

  ifdebug(1) {
    reference r = effect_any_reference(eff);
    pips_assert("Effect eff is consistent", effect_consistent_p(eff));
    pips_assert("The reference is consistent", reference_consistent_p(r));
  }

  if(anywhere_effect_p(eff))
    independent_p = TRUE;
  else {
    reference r = effect_any_reference(eff);
    entity v = reference_variable(r);
    type t = ultimate_type(entity_type(v));

    if(pointer_type_p(t)) {
      list inds = reference_indices(r);

      independent_p = ENDP(inds);
    }
    else {
      pips_assert("The reference is consistent", reference_consistent_p(r));

      independent_p = reference_with_constant_indices_p(r);
    }
  }

  return independent_p;
}

/* FI: I use this predicate to guard the use-def chains computation */
bool fortran_compatible_effect_p(effect e)
{
  bool compatible_p = FALSE;
  reference r = effect_any_reference(e);
  entity v = reference_variable(r);
  type ut = ultimate_type(entity_type(v));
  list ind = reference_indices(effect_any_reference(e));

  /* Basically, this is about a Fortran effect... */
  if((!pointer_type_p(ut) || ENDP(ind))) {
    compatible_p = TRUE;
  }

  return compatible_p;
}


/* Two effects interfere if one of them modify the set of locations
   defined by the other one. For instance, an index or a pointer may
   be used by one effect and changed by the other one.

   If a subscript expression is changed, the corresponding subscript
   must be replaced by an unbounded expression.

   If a pointer is written, any indirect effect thru this pointer must
   be changed into a read or write anywhere.

   This function is conservative: it is always correct to declare an interference.

   FI: I'm not sure what you can do when you know two effects interfere...
 */
bool effects_interfere_p(effect eff1, effect eff2)
{
  action ac1 = effect_action(eff1);
  action ac2 = effect_action(eff2);
  bool interfere_p = FALSE;

  if(action_write_p(ac1)||action_write_p(ac2)) {
    if(anywhere_effect_p(eff1) && action_write_p(ac1)) {
      interfere_p = !store_independent_effect_p(eff2);
    }
    else if(anywhere_effect_p(eff2) && action_write_p(ac2)) {
      interfere_p = !store_independent_effect_p(eff1);
    }
    else { /* dealing with standard effects */

      /* start with complex cases */
      /* The write effect is a direct effet, the other effect may be
	 direct or indirect, indexed or not. */
      reference wr = reference_undefined;
      entity wv = entity_undefined;
      reference rr = reference_undefined;
      entity rv = entity_undefined;

      list rind = list_undefined;
      list wind = list_undefined;

      if(action_write_p(ac1)) {
	wr = effect_any_reference(eff1);
	rr = effect_any_reference(eff2);
      }
      else {
	wr = effect_any_reference(eff2);
	rr = effect_any_reference(eff1);
      }

      wv = reference_variable(wr);
      wind = reference_indices(wr);
      rv = reference_variable(rr);
      rind = reference_indices(rr);

      /* Does the write impact the indices of the read? */
      MAP(EXPRESSION, s, {
	  list rl = NIL;

	  rl = expression_to_reference_list(s, rl);
	  MAP(REFERENCE, r, {
	      entity v = reference_variable(r);
	      if(wv==v) {
		interfere_p = TRUE;
		break;
	      }
	    }, rl);
	  if(interfere_p)
	    break;
	}, rind);

      //interfere_p = TRUE;
    }
  }
  return interfere_p;
}

effect effect_to_store_independent(effect eff)
{
  reference r = effect_any_reference(eff);
  entity v = reference_variable(r);
  type t = ultimate_type(entity_type(v));

  if(pointer_type_p(t)) {
    effect n_eff = anywhere_effect(copy_action(effect_action(eff)));
    free_effect(eff);
    eff = n_eff;
  }
  else{
    list ind = reference_indices(r);
    list cind = list_undefined;

    for(cind = ind; !ENDP(cind); POP(cind)) {
      expression e = EXPRESSION(CAR(cind));

      if(!unbounded_expression_p(e)) {
	if(!extended_integer_constant_expression_p(e)) {
	  free_expression(e);
	  EXPRESSION_(CAR(cind)) = make_unbounded_expression();
	}
      }
    }
  }

  return eff;
}

/* Modify eff so that the set of memory locations decribed after a
   write to some pointer p is still in the abstract location set of eff.

   \\n_eff
   p = ...;
   \\ eff of stmt s
   s;

   If p is undefined, assumed that any pointer may have been updated.

   As a pointer could be used in indexing, the current implementation
   is not correct/sufficient
 */
effect effect_to_pointer_store_independent_effect(effect eff, entity p)
{
  reference r = effect_any_reference(eff);
  entity v = reference_variable(r);
  //type t = ultimate_type(entity_type(v));

  if(entity_undefined_p(p) || v==p) {
    if(!ENDP(reference_indices(r))) {
      /* p[i][j] cannot be preserved */
      effect n_eff = anywhere_effect(copy_action(effect_action(eff)));
      free_effect(eff);
      eff = n_eff;
    }
    else {
      /* No problem: direct scalar reference */
      ;
    }
  }
  return eff;
}

/* Modify eff so that the set of memory locations decribed after a
   write to some non pointer variable is still in the abstract location set of eff.

   \\n_eff
   i = ...;
   \\ eff of stmt s: p[j], q[i],..
   s;
 */
effect effect_to_non_pointer_store_independent_effect(effect eff)
{
  reference r = effect_any_reference(eff);
  //entity v = reference_variable(r);
  //type t = ultimate_type(entity_type(v));

  r = reference_with_store_independent_indices(r);

  return eff;
}

  /* Modifies effect eff1 to make sure that any memory state
     modification abstracted by eff2 preserves the correctness of
     eff1: all memory locations included in eff1 at input are included
     in the memory locations abstracted by the new eff1 after the
     abstract state transition. */
effect effect_interference(effect eff1, effect eff2)
{
  //action ac1 = effect_action(eff1);
  action ac2 = effect_action(eff2);
  effect n_eff1 = eff1; /* default value */

  ifdebug(1) {
    pips_assert("The new effect is consistent", effect_consistent_p(eff1));
    pips_assert("The new effect is consistent", effect_consistent_p(eff2));
  }

  if(store_independent_effect_p(eff1)) {
    /* nothing to worry about */
    ;
  }
  else if(action_write_p(ac2)) {
    if(anywhere_effect_p(eff2)) {
      // free_effect(eff1);
      n_eff1 = effect_to_store_independent(eff1);
    }
    else {
      reference r2 = effect_any_reference(eff2);
      entity v2 = reference_variable(r2);
      type t2 = ultimate_type(entity_type(v2));

      if(pointer_type_p(t2)) {
	/* pointer-dependence write, indexed or not */
	n_eff1 = effect_to_pointer_store_independent_effect(eff1, v2);
      }
      else {
	/* The base address for the write is constant, the indices should be be checked */
	/* The write effect is a direct effet, the other effect may be
	   direct or indirect, indexed or not. */
	reference r1 = effect_any_reference(eff1);
	list ind1 = reference_indices(r1);
	list cind1 = list_undefined;

	/* FI: should be very similar to reference_with_store_independent_indices()? */

	/* Does the write impact some indices of the read? */
	for(cind1 = ind1; !ENDP(ind1); POP(ind1)) {
	  expression s = EXPRESSION(CAR(cind1));
	  list rl = NIL;
	  list crl = list_undefined;
	  bool interfere_p = FALSE;

	  rl = expression_to_reference_list(s, rl);
	  for(crl=rl; !ENDP(rl); POP(rl)) {
	    reference r = REFERENCE(CAR(crl));
	    entity v = reference_variable(r);
	    if(v2==v) {
	      interfere_p = TRUE;
	      break;
	    }
	  }

	  if(interfere_p) {
	    pips_debug(8, "Interference detected\n");
	    /* May be shared because of persistant references */
	    //free_expression(s);
	    EXPRESSION_(CAR(ind1)) = make_unbounded_expression();
	  }
	}
      }
    }
  }
  ifdebug(1)
    pips_assert("The new effect is consistent", effect_consistent_p(n_eff1));
  return n_eff1;
}

bool expression_invariant_wrt_effects(expression exp, list el)
{
  extern list proper_effects_of_expression(expression);
  list ee = proper_effects_of_expression(exp);
  list cee = list_undefined;
  list cel = list_undefined;
  bool invariant_p = TRUE;

  for(cee=ee; !ENDP(cee) && invariant_p; POP(cee)) {
    effect exp_e = EFFECT(CAR(cee));
    //reference exp_r = effect_any_reference(exp_e);
    //entity exp_v = reference_variable(exp_r);

    for(cel=el; !ENDP(el) && invariant_p; POP(el)) {
      effect l_e = EFFECT(CAR(cel));
      action l_a = effect_action(l_e);

      if(action_write_p(l_a)) {
	//reference l_r = effect_any_reference(l_e);
	//entity l_v = reference_variable(l_r);

	if(effects_interfere_p(l_e,exp_e)) {
	  invariant_p = FALSE;
	}
      }
    }
  }
  return invariant_p;
}

string action_to_string(action ac)
{
  return action_read_p(ac)? "read" : "write";
}

bool effects_write_variable_p(list el, entity v)
{
  bool result = FALSE;
  FOREACH(EFFECT, e, el) {
    action a  = effect_action(e);
    entity ev = effect_entity(e);
    if (action_write_p(a) && ev == v) {
      result = TRUE;
      break;
    }
  }
  return result;
}

bool effects_read_variable_p(list el, entity v)
{
  bool result = FALSE;
  FOREACH(EFFECT, e, el) {
    action a  = effect_action(e);
    entity ev = effect_entity(e);
    if (action_read_p(a) && ev == v) {
      result = TRUE;
      break;
    }
  }
  return result;
}

/* Check that all effects in el are read effects */
bool effects_all_read_p(list el)
{
  bool result = TRUE;
  FOREACH(EFFECT, e, el) {
    action a  = effect_action(e);
    //entity ev = effect_entity(e);
    if (action_write_p(a)) {
      result = FALSE;
      break;
    }
  }
  return result;
}

/* Check if some references might be freed with the effects. This may
   lead to disaster if the references are part of another PIPS data
   structure. This information is not fully accurate, but
   conservative. */
bool effect_list_can_be_safely_full_freed_p(list el)
{
  bool safe_p = TRUE;
  FOREACH(EFFECT, e, el) {
    cell c = effect_cell(e);
    if(cell_reference_p(c)) {
      reference r = cell_reference(c);
      list inds = reference_indices(r);

      if(ENDP(inds)) {
	/* The free is very likely to be unsafe */
	entity v = reference_variable(r);
	safe_p = FALSE;
	//fprintf(stderr, "cell_reference for %s", entity_name(v));
      }
      else {
	/* Is it a possible C reference or is it a synthetic reference
	   generated by the effect analysis? Hard to decide... */
	entity v = reference_variable(r);
	//fprintf(stderr, "cell_reference for %s", entity_name(v));
	print_reference(r);
      }
      break;
    }
  }
  return safe_p;
}
