 /* package semantics - prettyprint interface */

#include <stdio.h>
extern int fprintf();
#include <string.h>
/* #include <stdlib.h> */

#include "types.h"

#include "genC.h"

#include "text.h"
#include "text-util.h"

#include "constants.h"

#include "ri.h"
#include "ri-util.h"

#include "database.h"
#include "pipsdbm.h"
#include "resources.h"

#include "misc.h"
#include "properties.h"

#include "prettyprint.h"

#include "transformer.h"
#include "effects.h"

#include "semantics.h"

#define PREC_FORESYS_PREFIX "C$PREC"
#define TRAN_FORESYS_PREFIX "C$TRAN"
#define FORESYS_CONTINUATION_PREFIX "C$&"
#define PIPS_NORMAL_PREFIX "C"

DEFINE_CURRENT_MAPPING(semantic, transformer)

static bool is_transformer;
static bool is_user_view;
static hash_table nts = hash_table_undefined;

void print_code_transformers(module_name)
char *module_name;
{
    is_user_view = FALSE;
    is_transformer = TRUE;
    print_code_semantics(module_name);
}

void print_code_preconditions(module_name)
char *module_name;
{
    is_user_view = FALSE;
    is_transformer = FALSE;
    print_code_semantics(module_name);
}

void print_source_transformers(module_name)
char *module_name;
{
    is_user_view = TRUE;
    is_transformer = TRUE;
    print_code_semantics(module_name);
}

void print_source_preconditions(module_name)
char *module_name;
{
    is_user_view = TRUE;
    is_transformer = FALSE;
    print_code_semantics(module_name);
}

void print_code_semantics(module_name)
char *module_name;
{
    text r = make_text(NIL);
    char *filename;
    FILE *fd;
    entity mod;
    statement mod_stat;
    transformer summary = transformer_undefined;
    statement user_stat = statement_undefined;

    set_current_module_entity( local_name_to_top_level_entity(module_name) );
    mod = get_current_module_entity();

    set_current_module_statement( (statement)
	db_get_memory_resource(DBR_CODE, module_name, TRUE) );
    mod_stat = get_current_module_statement();

    if(is_user_view) {
	user_stat =  (statement)
	    db_get_memory_resource(DBR_PARSED_CODE, module_name, TRUE);

	debug_on("SEMANTICS_DEBUG_LEVEL");

	nts = allocate_number_to_statement();
	nts = build_number_to_statement(nts, mod_stat);

	ifdebug(1) {
	    print_number_to_statement(nts);
	}
	debug_off();
    }

    /* to set up the hash table to translate value into value names */
    set_cumulated_effects_map( effectsmap_to_listmap((statement_mapping) 
	db_get_memory_resource(DBR_CUMULATED_EFFECTS, module_name, TRUE)));
    module_to_value_mappings(mod);

    /* semantic information to print */
    set_semantic_map( (statement_mapping)
	db_get_memory_resource(
			       is_transformer? DBR_TRANSFORMERS 
			       : DBR_PRECONDITIONS,
			       module_name,
			       TRUE) );
    summary = (transformer)
	db_get_memory_resource(
			       is_transformer? DBR_SUMMARY_TRANSFORMER
			       : DBR_SUMMARY_PRECONDITION,
			       module_name,
			       TRUE);

    if(string_undefined_p((string) summary)) {
	pips_error("print_code_semantics",
		   "Summary information %s unavailable\n",
		   is_transformer? "DBR_SUMMARY_TRANSFORMER"
		   : "DBR_SUMMARY_PRECONDITION");
    }

    /* prepare the prettyprintting */
    filename = strdup(concatenate(db_get_current_program_directory(), 
				  "/",
				  module_name,
				  is_transformer?
				  (is_user_view? USER_TRANSFORMER_SUFFIX : SEQUENTIAL_TRANSFORMER_SUFFIX ) :
				  (is_user_view? USER_PRECONDITION_SUFFIX : SEQUENTIAL_PRECONDITION_SUFFIX),
				  NULL));
    fd = safe_fopen(filename, "w");

    init_prettyprint(semantic_to_text);

    debug_on("SEMANTICS_DEBUG_LEVEL");



    /* initial version; to be used again when prettyprint really prettyprints*/
    /* print_text(fd, text_statement(mod, 0, mod_stat)); */

    /* new version */

    /* summary information first */
    MERGE_TEXTS(r,text_transformer(summary)); 

    debug_off();

    /* then code with the corresponding information */
    ADD_SENTENCE_TO_TEXT(r, 
			 make_sentence(is_sentence_formatted, 
				       code_decls_text(entity_code(mod))));
    MERGE_TEXTS(r, text_statement(mod, 0, is_user_view? user_stat:mod_stat));
    ADD_SENTENCE_TO_TEXT(r, sentence_tail());
    print_text(fd, r);
    /* end of new version */

    safe_fclose(fd, filename);

    /* Let's assume that value_mappings will be somehow sometime freed... */

    DB_PUT_FILE_RESOURCE(is_user_view? DBR_PARSED_PRINTED_FILE : DBR_PRINTED_FILE,
			 strdup(module_name),
 			 filename);

    if(is_user_view) {
	hash_table_free(nts);
	nts = hash_table_undefined;
    }
    reset_cumulated_effects_map();
    reset_semantic_map();
    reset_current_module_entity();
    reset_current_module_statement();
}

/* this function name is VERY misleading - it should be changed, sometime FI */
text semantic_to_text(module, margin, stmt)
entity module;
int margin;
statement stmt;
{
    transformer t;
    text txt;

    if(is_user_view) {
	statement i = (statement) hash_get(nts, (char *) statement_number(stmt));

	if(i!=(statement) HASH_UNDEFINED_VALUE) {
	    t = load_statement_semantic(i);
	}
	else
	    t = (transformer) HASH_UNDEFINED_VALUE;
    }
    else
	t = load_statement_semantic(stmt);

    txt = text_transformer(t);
    return txt; 
}


/* text text_transformer(transformer tran) 
 * input    : a transformer representing a transformer or a precondition 
 * output   : a text containing commentaries representing the transformer
 * modifies : nothing.
 */
text text_transformer(tran)
transformer tran;
{
    text txt = make_text(NIL);
    boolean foresys = get_bool_property("PRETTYPRINT_FOR_FORESYS");
    string str_tran, str_prefix;

    if(tran != (transformer) HASH_UNDEFINED_VALUE) {
	if (is_transformer){
	    str_tran = transformer_to_string(tran);
	    if (foresys) 
		str_prefix = TRAN_FORESYS_PREFIX;
	    else 
		str_prefix = PIPS_NORMAL_PREFIX;
	}
	else { 
	    str_tran = precondition_to_string(tran);
	    if (foresys) 
		str_prefix = PREC_FORESYS_PREFIX;
	    else 
		str_prefix = PIPS_NORMAL_PREFIX;
	}
	
	ADD_SENTENCE_TO_TEXT(txt, make_sentence(is_sentence_formatted, strdup("\n")));
	MERGE_TEXTS(txt, 
		    string_predicate_to_commentary(str_tran, str_prefix));
	ADD_SENTENCE_TO_TEXT(txt, make_sentence(is_sentence_formatted, strdup("\n")));
    }
    return txt; 
}


/* ---------------------------------------------------------------- */
/* to convert strings containing predicates to text of commentaries */
/* BA, april 1994                                                   */
/* ---------------------------------------------------------------- */

#define MAX_PRED_COMMENTARY_STRLEN 70


/* text string_predicate_to_commentary(string str_pred, string comment_prefix) 
 * input    : a string, part of which represents a predicate.
 * output   : a text consisting of several lines of commentaries,
 *            containing the string str_pred, and beginning with 
 *            comment_prefix.
 * modifies : str_pred;
 */
text string_predicate_to_commentary(str_pred, comment_prefix)
string str_pred;
string comment_prefix;
{
    text t_pred = make_text(NIL);
    string str_suiv = NULL;
    string str_prefix = comment_prefix;
    char str_tmp[MAX_PRED_COMMENTARY_STRLEN];
    int len, new_str_pred_len, longueur_max;
    boolean premiere_ligne = TRUE;
    boolean foresys = get_bool_property("PRETTYPRINT_FOR_FORESYS");
    longueur_max = MAX_PRED_COMMENTARY_STRLEN - strlen(str_prefix) - 2;

    /* if str_pred is too long, it must be splitted in several lines; 
     * the hyphenation must be done only between the constraints of the
     * predicate, when there is a "," or a ")". A space is added at the beginning
     * of extra lines, for indentation. */
    while((len = strlen(str_pred)) > 0) {
	if (len > longueur_max) {

	    /* search the maximal substring which length 
	     * is less than longueur_max */
	    str_tmp[0] = '\0';
	    (void) strncat(str_tmp, str_pred, longueur_max);

	    switch (foresys) {
	    case FALSE : 
		str_suiv = strrchr(str_tmp, ',');
		break;
	    case TRUE : 
		str_suiv = strrchr(str_tmp, ')');
		break;
	    }
	    new_str_pred_len = (strlen(str_tmp) - strlen(str_suiv)) + 1;
	    str_suiv = strdup(&(str_pred[new_str_pred_len]));

	    str_tmp[0] = '\0';
	    if (!premiere_ligne)
		(void) strcat(str_tmp, " "); 
	    (void) strncat(str_tmp, str_pred, new_str_pred_len);

	    /* add it to the text */
	    ADD_SENTENCE_TO_TEXT(t_pred, 
				 make_pred_commentary_sentence(strdup(str_tmp), str_prefix));
	    str_pred =  str_suiv;
	}
	else {
	    /* if the remaining string fits in one line */
	    str_tmp[0] = '\0';
	    if (!premiere_ligne)
		(void) strcat(str_tmp, " "); 
	    (void) strcat(str_tmp, str_pred);

	    ADD_SENTENCE_TO_TEXT(t_pred, 
				 make_pred_commentary_sentence(str_tmp, str_prefix));
	    str_pred[0] = '\0';
	}
	
	if (premiere_ligne) {
	    premiere_ligne = FALSE;
	    longueur_max = longueur_max - 1;
	    if (foresys){
		int i, nb_espaces = strlen (str_prefix) - strlen(FORESYS_CONTINUATION_PREFIX);
		str_prefix = strdup(str_prefix);
		str_prefix[0] = '\0';
		(void) strcat(str_prefix, FORESYS_CONTINUATION_PREFIX);
		for (i=1; i <= nb_espaces; i++)
		    (void) strcat(str_prefix, " ");
	      }
	  }
    }
    
    return(t_pred);
}
    


/* text words_predicate_to_commentary(list w_pred, string comment_prefix)
 * input    : a list of strings, one of them representing a predicate.
 * output   : a text of several lines of commentaries containing 
 *            this list of strings, and beginning with comment_prefix.
 * modifies : nothing.
 */
text words_predicate_to_commentary(w_pred, comment_prefix)
list w_pred;
string comment_prefix;
{
    string str_pred;
    text t_pred;

    /* str_pred is the string corresponding to the concatenation
     * of the strings in w_pred */
    str_pred = words_to_string(w_pred);

    t_pred = string_predicate_to_commentary(str_pred, comment_prefix);

    return(t_pred);
}


/* sentence make_pred_commentary_sentence(string str_pred, string comment_prefix) 
 * input    : a substring formatted to be a commentary
 * output   : a sentence, containing the commentary form of this string,
 *            beginning with the comment_prefix.
 * modifies : nothing
 */
sentence make_pred_commentary_sentence(str_pred, comment_prefix)
string str_pred;
string comment_prefix;
{
    char str_tmp[MAX_PRED_COMMENTARY_STRLEN + 1];
    sentence sent_pred;

    str_tmp[0] = '\0';
    (void) strcat(str_tmp, comment_prefix); 
    (void) strcat(str_tmp, "  ");
    (void) strcat(str_tmp, str_pred);
    (void) strcat(str_tmp, "\n"); 

    sent_pred = make_sentence(is_sentence_formatted, strdup(str_tmp));
    free(str_tmp);
    return(sent_pred);
}

