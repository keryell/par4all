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
/*
 * Fabien COELHO
 */

#include "local-header.h"

/* these functions must translate external reductions into local ones if 
 * must be. thus it is similar to summary effects call translations.
 * I do reuse some functions there by generating fake effects...
 * I think they should be cleaned so as to offer reference plus predicate
 * translations, and then would be used by reductions and effects.
 * Fabien.
 */

/* translation of a reference, based on effect translations...
 * such a interface should be available?
 */
list /* of reference */ 
summary_to_proper_reference(
    call c,
    reference r)
{
    effect e = make_simple_effect(r, /* persistent! */
			   make_action(is_action_write, UU),
			   make_approximation(is_approximation_must, UU));
    list /* of effect */ lef, /* of reference */ lref = NIL;
    
    pips_debug(7, "reference to %s\n", entity_name(reference_variable(r)));

    debug_on("EFFECTS_DEBUG_LEVEL");
    lef = summary_effect_to_proper_effect(c, e);
    debug_off();
    
    MAP(EFFECT, ef, 
	lref = CONS(REFERENCE, effect_reference(ef), lref),
	lef);
    
    gen_map(gen_free, lef), gen_free_list(lef);
    return lref;
}

static list /* of reduction */
translate_reduction(
    call c,
    reduction external_red)
{
    reference ref = reduction_reference(external_red);
    entity var = reference_variable(ref);
    list /* of reference */ lref = summary_to_proper_reference(c, ref),
         /* of reduction */ lrds = NIL;

    pips_debug(7, "reduction %s[%s] (%td reductions)\n", 
	       reduction_name(external_red),
	       entity_name(var), gen_length(lref));

    MAP(REFERENCE, r, 
    {
	reduction red = copy_reduction(external_red);

	free_reference(reduction_reference(red));
	reduction_reference(red) = copy_reference(r);

	gen_free_list(reduction_dependences(red));
	reduction_dependences(red) = NIL;

	gen_map(gen_free, reduction_trusted(red));
	gen_free_list(reduction_trusted(red)); 
	reduction_trusted(red) = NIL;

	/* ??? what about effects on commons hidden in the call?
	 * I do not know the reference to trust in the effects...
	 */
	reduction_trusted(red) = CONS(PREFERENCE, make_preference(r), NIL);
	
	pips_debug(7, "reduction on %s translated on %s\n", 
		   entity_name(var), entity_name(reduction_variable(red)));

	lrds = CONS(REDUCTION, red, lrds);
    },
	lref);

    gen_free_list(lref); /* just the backbone, refs are real in the code? */
    return lrds;
}

list /* of reduction */
translate_reductions(
    call c)
{
    entity fun = call_function(c);
    list /* of reduction */ lr = NIL;

    if (!entity_module_p(fun))
	return NIL;

    MAP(REDUCTION, r, 
	lr = gen_nconc(translate_reduction(c, r), lr),
	reductions_list(load_summary_reductions(fun)));

    return lr;
}
   

/* end of it
 */
