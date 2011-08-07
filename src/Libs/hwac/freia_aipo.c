/*

  $Id$

  Copyright 1989-2011 MINES ParisTech

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

#include <stdint.h>
#include <stdlib.h>

#include "genC.h"
#include "misc.h"

#include "linear.h"

#include "ri.h"

#include "freia.h"
#include "freia_spoc_private.h"
#include "hwac.h"

list freia_aipo_compile_calls
(string module,
 list /* of statements */ ls,
 const hash_table occs,
 const set output_images,
 int number)
{
  // build DAG for ls
  pips_debug(3, "considering %d statements\n", (int) gen_length(ls));
  pips_assert("some statements", ls);

  dag fulld = freia_build_dag(module, ls, number, occs, output_images);
  list added_stats = freia_dag_optimize(fulld);

  // intermediate images
  hash_table init = hash_table_make(hash_pointer, 0);
  list new_images = dag_fix_image_reuse(fulld, init, occs);

  // dump final optimised dag
  dag_dot_dump_prefix(module, "dag_cleaned_", number, fulld);

  // now may put actual allocations, which messes up statement numbers
  list reals = freia_allocate_new_images_if_needed(ls, new_images, occs, init);

  // ??? append possibly extracted copies?
  // should it be NIL because it is not useful in AIPO->AIPO?
  freia_insert_added_stats(ls, added_stats);

  // cleanup
  gen_free_list(new_images);
  hash_table_free(init);
  free_dag(fulld);

  return reals;
}