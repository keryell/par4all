/* PACKAGE MOVEMENTS
 *
 * Corinne Ancourt  - septembre 1991
 */

#include <stdio.h>



#include "genC.h"
#include "ri.h"
#include "ri-util.h"
#include "misc.h"

#include "tiling.h"
#include "matrice.h"
#include "movements.h"

/* this function builds the following system of constraints depending
 *  on the machine. It describes the implementation of array elements in
 * the memory in function of bank, ligne size, ligne bank,...
 * 
 * bn is the number of banks, ls the ligne size, ms the first array dimension
 * bank is a variable giving the bank id., ligne a variable corresponding to
 * a  ligne  in the bank, ofs a variable corresponding to an offset 
 * in a ligne of the bank. 
 *
 * if COLUMN_MAJOR is TRUE the system is the following one
 *
 *      (VAR1-1) + (VAR2-1) *ms == bn*ls* ligne +ls*bank+ofs,
 *        1 <= bank <= bn ,
 *        1 <= proc <= pn ,
 *        0 <= ofs <= ls-1
 *
 *else it is
 *
 *      (VAR1-1) * ms + (VAR2-1) == bn*ls*ligne +ls*bank+ofs,
 *        1 <= bank <= bn ,
 *        1 <= proc <= pn ,
 *        0 <= ofs <= ls-1
*/

Psysteme build_sc_machine(pn,bn,ls,sc_array_function,proc_id,bank_indices,entity_var)
int pn,bn,ls;
Psysteme sc_array_function;
entity proc_id;
Pbase bank_indices;
entity entity_var;
{

    type t = entity_type(entity_var);
    int ms=0;
    Variable vbank,vligne,vofs;
    Psysteme sc = sc_init_with_sc(sc_array_function);
    Pcontrainte pc;
    Pvecteur pv1,pv2;
    int nb_bytes=1;
    basic bas;

    debug(8,"build_sc_machine","begin\n");
    if (type_variable_p(t)) {
	variable var = type_variable(t);
	cons * dims = variable_dimensions(var);
	dimension dim1 = DIMENSION(CAR(dims));
	expression lower= dimension_lower(dim1);
	normalized norm1 = NORMALIZE_EXPRESSION(lower);
	expression upper= dimension_upper(dim1);
	normalized norm2 = NORMALIZE_EXPRESSION(upper);
	int min_ms =0;
	int max_ms=0;
	if (normalized_linear_p(norm1) && normalized_linear_p(norm2)) {
	    min_ms = vect_coeff(TCST,(Pvecteur) normalized_linear(norm1));
	    max_ms = vect_coeff(TCST,(Pvecteur) normalized_linear(norm2));
	}
	ms = max_ms - min_ms +1;

	bas = variable_basic(var);
	/* Si l'on veut utiliser le nombre d'octets il faut remplacer l'equation 
	   par deux inequations du type 
	   
	   if COLUMN_MAJOR is TRUE the system is the following one
	   
	   (VAR1-1) + (VAR2-1) *ms <= bn*ls* (ligne-1) +ls*(bank-1)+ofs,
	   bn*ls* (ligne-1) +ls*(bank-1)+ofs <=  (VAR1) + (VAR2-1) *ms

	   else it is
	   
	   (VAR1-1) * ms + (VAR2-1) <= bn*ls*(ligne-1) +ls*(bank-1)+ofs,
	   bn*ls*(ligne-1) +ls*(bank-1)+ofs <=  (VAR1-1) * ms + (VAR2)

	   */

	/*	  nb_bytes = SizeOfElements(bas);*/
	
    }

    ifdebug(8) { (void) fprintf(stderr," MS = %d \n",ms); }

    vbank = vecteur_var(bank_indices);
    vligne = vecteur_var(bank_indices->succ);
    vofs = vecteur_var(bank_indices->succ->succ);
    sc->base = vect_add_variable(sc->base,vbank);
    sc->base = vect_add_variable(sc->base,vligne);
    sc->base = vect_add_variable(sc->base,vofs);

    sc->dimension +=3;

    /* bank_indices is assumed to belong the three variables
       bank_id, L and O (see documentation for more details) */

    /* if COLUMN_MAJOR is TRUE then build the constraint   
       (VAR1-1) + (VAR2-1) *ms == bn*ls*L +ls*bank_id+O,
       else build the constraint
       (VAR1-1) * ms + (VAR2-1) == bn*ls*L +ls*bank_id+O,
       VAR1 and VAR2 correspond to the image array function indices */
    pv1 = vect_new(vbank,-ls);
    vect_add_elem(&pv1,vligne,-bn*ls);
    vect_add_elem(&pv1,vofs,-1);
    if (COLUMN_MAJOR)
	pc = sc_array_function->inegalites;
    else pc = sc_array_function->inegalites->succ;
    /* to deal with MONO dimensional array */
    if (pc==NULL) pc= contrainte_make(vect_new(TCST,1));
    pv2 = vect_dup(pc->vecteur);
    vect_add_elem(&pv2,TCST,-1);
    pv2 = vect_multiply(pv2,nb_bytes);
    pv1 = vect_add(pv1,pv2);
    if (COLUMN_MAJOR)
	pc = pc->succ;
    else pc =  sc_array_function->inegalites;    
    /* to deal with MONO dimensional array */
    if (pc==NULL) pc=  contrainte_make(vect_new(TCST,1));
    pv2 = vect_dup(pc->vecteur);
    vect_add_elem(&pv2,TCST,-1);
    pv2 = vect_multiply(pv2,ms * nb_bytes);
    pv1 = vect_add(pv1,pv2);
    pc = contrainte_make(pv1);
    sc_add_eg(sc,pc);

    /* build the constraints 0 <= bank_id <= bn-1 */

    pv2 = vect_new(vbank, -1);
    pc = contrainte_make(pv2);
    sc_add_ineg(sc,pc);
    pv2 = vect_new(vbank, 1);
    vect_add_elem(&pv2,TCST,- bn+1);
    pc = contrainte_make(pv2);
    sc_add_ineg(sc,pc);

    /* build the constraints 0 <= proc_id <= pn-1 */
    sc->base = vect_add_variable(sc->base,(char *) proc_id);
    sc->dimension++;
    pv2 = vect_new((char *) proc_id, -1);
    pc = contrainte_make(pv2);
    sc_add_ineg(sc,pc);
    pv2 = vect_new((char *) proc_id, 1);
    vect_add_elem(&pv2,TCST,- pn+1);
    pc = contrainte_make(pv2);
    sc_add_ineg(sc,pc);


    /* build the constraints 0 <= O <= ls -1 */

    pv2 = vect_new(vofs, -1);
    pc = contrainte_make(pv2);
    sc_add_ineg(sc,pc);
    pv2 = vect_new(vofs, 1);
    vect_add_elem(&pv2,TCST,- ls +1);
    pc = contrainte_make(pv2);
    sc_add_ineg(sc,pc);


    /* build the constraints 0 <= L   */
 
 
    pc = contrainte_make(vect_new(vligne,-1));
    sc_add_ineg(sc,pc);
    ifdebug(8)  {
	(void) fprintf(stderr,"Domain Machine :\n");
	sc_fprint(stderr,sc,entity_local_name);    }
    debug(8,"build_sc_machine","end\n");

    return(sc);

}





