/* bourdoncle.c

   Decomposition of a CFG into SCC's and sub-SCC's. Algorithm 3.9, Page
   43, Francois Bourdoncle's PhD. Define heuristically SCC and sub-SCC
   heads.

   Build a set nof new data structures to represent any CFG as a recursive
   stucture of CFG's and DAG's.

   Some predecessors or successors must be deleted in the recursive
   descent. These vertices are given a statement_undefined statement and
   empty lists of predecessors and successors. Empty successors must be
   maintained so as to know if they are true or false successors. I
   suppose that we do not really care about irrelevant predecessors and
   that we might as well delete them. In fact, only the true branch
   successor absolutely must be preserved.

   Since the new CFG's are not deterministic, the number of successors is
   not bounded to two and any statement can have more than one
   successor. Standard consistency check are likely to fail if applied to
   the unstructured produced by this decomposition.

 $Id$

 $Log: bourdoncle.c,v $
 Revision 1.2  2002/06/04 16:07:04  irigoin
 Lots of new development. Still not working recursively. Only the deepest
 cycle in unstruc10.f is handled.

 Revision 1.1  2002/05/28 15:04:03  irigoin
 Initial revision */

#include <stdio.h>
#include <strings.h>

#include "linear.h"

#include "genC.h"
#include "ri.h"
#include "ri-util.h"
#include "control.h"
/* #include "properties.h" */

#include "misc.h"

/* #include "constants.h" */


/* Data structures for Bourdoncle's heuristics:
 *
 * dfn = depth first number
 *
 * num = current vertex number
 *
 * vertex_stack = stack of visited nodes
 *
 */

static hash_table dfn = hash_table_undefined;

static void reset_dfn(control c)
{
  hash_put(dfn, (void *) c, (void *) 0);
}

static int get_dfn(control c)
{
  int d = 0;
  
  if((d = (int) hash_get(dfn, (void *) c)) == (int) (HASH_UNDEFINED_VALUE))
    pips_internal_error("No dfn value for control %p\n", c);

  return d;
}

static void update_dfn(control c, int d)
{
  hash_update(dfn, (void *) c, (void *) d);
}


static int num = 0;

DEFINE_LOCAL_STACK(vertex, control)


control make_meaningless_control(list preds, list succs)
{
  control c =  make_control(statement_undefined, preds, succs);
  return c;
}

bool meaningless_control_p(control c)
{
  return statement_undefined_p(control_statement(c));
}

void free_meaningless_control(control c)
{
  /* free_control() cannot be used because you do not want to free the
     successors and the predecessors */
  pips_assert("No statement is associated", statement_undefined_p(control_statement(c)));
  gen_free_list(control_predecessors(c));
  gen_free_list(control_successors(c));
  control_predecessors(c)= list_undefined;
  control_successors(c)= list_undefined;
  free_control(c);
}


     /* Code partly replicated from semantics/unstructured.c to be able to
        taylor it to the exact needs. */

static void print_control_node(control c)
{
  fprintf(stderr,
	  "ctr %p, %d preds, %d succs: %s", 
          c,
	  gen_length(control_predecessors(c)),
	  gen_length(control_successors(c)),
	  safe_statement_identification(control_statement(c)));
  fprintf(stderr,"\tsuccessors:\n");
  MAP(CONTROL, s, {
    fprintf(stderr, "\t\t%p %s", s,
	    safe_statement_identification(control_statement(s)));
  }, control_successors(c));
  fprintf(stderr,"\tpredecessors:\n");
  MAP(CONTROL, p, {
    fprintf(stderr, "\t\t%p %s", p,
	    safe_statement_identification(control_statement(p)));
  }, control_predecessors(c));
  fprintf(stderr, "\n");
}

static void print_control_nodes(list l)
{
  MAP(CONTROL, c, {
    fprintf(stderr, "%p, %s", c,
	    safe_statement_identification(control_statement(c)));
  }, l);
  fprintf(stderr, "\n");
}

static void print_unstructured(unstructured u)
{
  pips_debug(2, "Unstructured %p with nodes:\n", u);
  /* Do not go down into nested unstructured */
  gen_multi_recurse(u, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, print_control_node, NULL);
  pips_debug(2, "With entry nodes\n");
  print_control_node(unstructured_control(u));
  pips_debug(2, "And exit node\n");
  print_control_node(unstructured_exit(u));
}

static list embedding_control_list = NIL;

void add_control_to_embedding_control_list(control c)
{
  embedding_control_list = CONS(CONTROL, c, embedding_control_list);
}

static void print_embedding_graph(control c)
{
  ifdebug(2) {
    bool consistent_embedding_graph_p = TRUE;

    pips_debug(2, "Embedding graph for vertex %p:\n", c);
    /* Do not go down into nested unstructured */
    gen_multi_recurse(c, statement_domain, gen_false, gen_null,
		      control_domain, gen_true, print_control_node, NULL);

    /* Check its structure: make sure that each node is a successor of its
       predecessors and a predecessor of all its successors */
    pips_assert("The embedding control list is NIL", embedding_control_list == NIL);
    gen_multi_recurse(c, statement_domain, gen_false, gen_null,
		      control_domain, gen_true, add_control_to_embedding_control_list, NULL);
    MAP(CONTROL, ec, {
      MAP(CONTROL, pred, {
	if(!gen_in_list_p(ec, control_successors(pred))){
	  consistent_embedding_graph_p = FALSE;
	  fprintf(stderr, "Control %p is a predecessor of control %p "
		  "but control %p is not a successor of control %p\n",
		  pred, ec, ec, pred);
	}
	
      }
	  , control_predecessors(ec));
      MAP(CONTROL, succ, {
	if(!gen_in_list_p(ec, control_predecessors(succ))){
	  consistent_embedding_graph_p = FALSE;
	  fprintf(stderr, "Control %p is a successor of control %p "
		  "but control %p is not a predecessor of control %p\n",
		  succ, ec, ec, succ);
	}
	
      }
	  , control_successors(ec));
     }
	, embedding_control_list);
    pips_assert("The embedding graph is consistent", consistent_embedding_graph_p);
    gen_free_list(embedding_control_list);
    embedding_control_list = NIL;
    pips_debug(2, "End: the embedding graph for vertex %p is consistent.\n", c);
  }
}

/* Allocate a list of control nodes transitively linked to control c */
static list node_to_linked_nodes(control c)
{
  list l = list_undefined;
  
  pips_debug(2, "Linked nodes for vertex %p:\n", c);
  pips_assert("The embedding control list is NIL", embedding_control_list == NIL);

  gen_multi_recurse(c, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, add_control_to_embedding_control_list, NULL);
  l = embedding_control_list;
  embedding_control_list = NIL;

  ifdebug(2) {
    pips_debug(2, "End with list:\n");
    print_control_nodes(l);
  }

  return l;
}

static void print_control_to_control_mapping(string message, hash_table map)
{
  fprintf(stderr, "%s\n", message);
  HASH_MAP(c1, c2, 
  {
    fprintf(stderr, "%p -> %p\n", c1, c2);
  }
	  , map);
  fprintf(stderr, "\n");
}


/* Functions to deal with the ancestor_map */

static bool ancestor_control_p(hash_table ancestor_map, control c)
{
  bool ancestor_p = FALSE;
  
  HASH_MAP(c_new, c_old, 
  {
    if(c_old==(void *) c)
      ancestor_p = TRUE;
  }
	   , ancestor_map);
  return ancestor_p;
}

/* If vertex is an ancestor control node from the input control graph,
   return vertex, else return its ancestor */
static control control_to_ancestor(control vertex, hash_table ancestor_map)
{
  control ancestor = control_undefined; 

  if((ancestor = hash_get(ancestor_map, vertex))==HASH_UNDEFINED_VALUE)
    ancestor = vertex;
  ifdebug(2) {
    /* This assert may be too strong, but all control nodes are copied
       when entering bourdoncle partition which should make it correct. */
    pips_assert("ancestor really is an ancestor",
		ancestor_control_p(ancestor_map, ancestor));
    }
  
  return ancestor;
}

static bool registered_controls_p(hash_table ancestor_map, list l)
{
  bool registered_p = TRUE;

  MAP(CONTROL, c, 
  {
    if(!meaningless_control_p(c)) {
      if(hash_get(ancestor_map, c)==HASH_UNDEFINED_VALUE) {
	if(!ancestor_control_p(ancestor_map, c)) {
	  ifdebug(2) {
	    pips_debug(2, "Control %p is not registered:\n", c);
	    print_control_node(c);
	  }
	  registered_p = FALSE;
	}
      }
    }
  }
      , l);
  return registered_p;
}

/* No control can be an ancestor and a child */
static bool ancestor_map_consistent_p(hash_table ancestor_map)
{
  bool check_p = TRUE;
  list  ancestors = NIL;
  list children = NIL;
  
  HASH_MAP(c_new, c_old, 
  {
    ancestors = gen_once((control) c_old, ancestors);
    children = CONS(CONTROL, (control) c_new, children);
  }
	   , ancestor_map);
  gen_list_and(&ancestors, children);
  check_p = ENDP(ancestors);

  ifdebug(2) {
    if(!ENDP(ancestors)) {
      pips_debug(2, "Bug some control are children and ancestors:\n");
      print_control_nodes(ancestors);
      print_control_to_control_mapping("Child to ancestor map:", ancestor_map);
    }
  }

  gen_free_list(ancestors);
  gen_free_list(children);
  
  return check_p;
}

static void add_child_parent_pair(hash_table ancestor_map,
				  control child,
				  control parent)
{
  control ancestor = control_undefined;

  /* The child will inherit the parent's ancestor, if any */
  if((ancestor = (control) hash_get(ancestor_map, parent))==(control) HASH_UNDEFINED_VALUE) {
    ancestor = parent;
  }
  ifdebug(2) {
    /* The child should not already be in ancestor_map */
    pips_assert("The child should not already be in ancestor_map",
		hash_get(ancestor_map, child)==HASH_UNDEFINED_VALUE);
    /* The child cannot be an ancestor */
    pips_assert("The child is not an ancestor", !ancestor_control_p(ancestor_map, child));
  }
  hash_put(ancestor_map, (void *) child, (void *) ancestor);
}

/* Replication of unstructured (i.e. CFG) and control nodes
   (i.e. vertices) */

static hash_table replicate_map = hash_table_undefined;

static void print_replicate_map()
{
  fprintf(stderr,"Dump of replicate_map:\n");
  HASH_MAP(old_c, new_c, 
  {
    fprintf(stderr, "Old %p -> New %p\n", old_c, new_c);
  }
	   , replicate_map);
  fprintf(stderr,"\n");
}

control control_shallow_copy(control c)
{
  control new_c = control_undefined;

  new_c = make_control(control_statement(c),
		       gen_copy_seq(control_predecessors(c)),
		       gen_copy_seq(control_successors(c)));
  hash_put(replicate_map, (void *) c, (void *) new_c);
  
  return new_c;
}

static void control_translate_arcs(control c)
{
  MAPL(c_c,
  {
    control old_c = CONTROL(CAR(c_c));
    control new_c = hash_get(replicate_map, (void *) old_c);
    pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
    CONTROL(CAR(c_c)) = new_c;
  }
      , control_predecessors(c));
  MAPL(c_c,
  {
    control old_c = CONTROL(CAR(c_c));
    control new_c = hash_get(replicate_map, (void *) old_c);
    pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
    CONTROL(CAR(c_c)) = new_c;
  }
      , control_successors(c));
}

static control control_to_replicate(control old_c)
{
  control new_c =  hash_get(replicate_map, (void *) old_c);
  pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
  return new_c;
}

unstructured unstructured_shallow_copy(unstructured u, hash_table ancestor_map)
{
  unstructured new_u = unstructured_undefined;
  
  pips_assert("replicate_map is undefined",
	      hash_table_undefined_p(replicate_map));
  
  replicate_map = hash_table_make(hash_pointer, 0);

  /* Do not go down into nested unstructured and replicate all control
     nodes */
  gen_multi_recurse(u, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, control_shallow_copy, NULL);

  ifdebug(2) print_replicate_map();

  /* Update predecessor and successor arcs of the new control nodes to
     point to the new control nodes using the global conversion mapping
     replicate_map */
  HASH_MAP(old_c, new_c,
  {
    control_translate_arcs((control) new_c);
  }
	   , replicate_map);

  /* Generate new unstructured with relevant entry and exit nodes */
  new_u = make_unstructured(control_to_replicate(unstructured_control(u)),
			    control_to_replicate(unstructured_exit(u)));

  /* Generate ancestor_map as the inverse function of replicate_map */
  HASH_MAP(old_n, new_n,{
    add_child_parent_pair(ancestor_map, (control) new_n, (control) old_n);
  }, replicate_map);
  
  hash_table_free(replicate_map);
  replicate_map = hash_table_undefined;

  return new_u;
}


/* Build a new unstructured (CFG) for partition. vertex is the entry and
   exit point. New nodes must be allocated because the parent graph is
   untouched. vertex is supposed to be included into partition. */
unstructured partition_to_unstructured(control vertex, list partition)
{
  unstructured new_u = unstructured_undefined;
  list useless = NIL;
  
  replicate_map = hash_table_make(hash_pointer, 0);

  /* Create the translation table replicate_map */
  MAP(CONTROL, c,
  {
    (void) control_shallow_copy(c);
  }
      , partition);

  /* Generate new unstructured with relevant entry and exit node vertex */
  new_u = make_unstructured(control_to_replicate(vertex),
			    control_to_replicate(vertex));

  /* Update arcs */

  /* Update predecessors */
  MAP(CONTROL, c,
  {
    control c_new = control_to_replicate(c);

    /* The only predecessors that are useful are in partition */
    MAPL(c_c,
    {
      control old_c = CONTROL(CAR(c_c));
      control new_c = control_undefined;

      if(gen_in_list_p(old_c, partition)) {
	new_c = hash_get(replicate_map, (void *) old_c);
	pips_assert("new_c is defined", new_c!=(control) HASH_UNDEFINED_VALUE);
	CONTROL(CAR(c_c)) = new_c;
      }
      else {
	/* This predecessor is irrelevant */
	useless = CONS(CONTROL, old_c, useless);
      }
    }
	 , control_predecessors(c_new));

    gen_list_and_not(&control_predecessors(c_new), useless);
    gen_free_list(useless);
    useless = NIL;

    /* Handle successors */
    MAPL(c_c,
    {
      control old_c = CONTROL(CAR(c_c));
      control new_c = control_undefined;

      if(gen_in_list_p(old_c, partition)) {
	new_c = hash_get(replicate_map, (void *) old_c);
	pips_assert("new_c is defined",
		    new_c!=(control) HASH_UNDEFINED_VALUE);
      }
      else {
	pips_assert("If a successor is not in partition,"
		    " then there must be two successors",
		    gen_length(control_successors(c_new))==2)
	/* This successor is irrelevant but irrelevant true branches must
           be preserved */
	if(c_c==control_successors(c_new)) {
	  pips_assert("The second successor is in partition and has been copied",
		      gen_in_list_p(CONTROL(CAR(CDR(c_c))),partition) );
	  new_c = make_meaningless_control(CONS(CONTROL, c_new, NIL), NIL);
	}
	else {
	  /* A second irrelevant successor is not needed. But are we ready
             to have IF nodes with only one successor? */
	  /* useless = CONS(CONTROL, old_c, useless); */
	  new_c = make_meaningless_control(CONS(CONTROL, c_new, NIL), NIL);
	}
      }
	
      if(!control_undefined_p(new_c))
	CONTROL(CAR(c_c)) = new_c;
    }
	 , control_successors(c_new));
  }
      , partition);

  /*
    control_predecessors(c_new) = gen_list_and_not(control_predecessors(c_new), useless);
    gen_free_list(useless);
    useless = NIL;
  */

  hash_table_free(replicate_map);
  replicate_map = hash_table_undefined;

  return new_u;
}

static bool entry_or_exit_control_p(control c,
				    list partition,
				    bool check_entry)
{
  bool is_entry_or_exit_p = FALSE;
  list nodes = check_entry?control_predecessors(c):control_successors(c);

  MAP(CONTROL, pred, 
  {
    if(!gen_in_list_p(pred, partition)) {
      is_entry_or_exit_p = TRUE;
      break;
    }
  }
      , nodes);

  return is_entry_or_exit_p;
}
static bool entry_control_p(control c, list partition)
{
  return entry_or_exit_control_p(c, partition, TRUE);
}

static bool exit_control_p(control c, list partition)
{
  return entry_or_exit_control_p(c, partition, FALSE);
}


/* If complement_p, preserve successors in the complement of
   partition. Else preserve successors in partition. Take care of test
   nodes wich must keep two successors no matter what. */
void intersect_successors_with_partition_or_complement(control c,
						       list partition,
						       bool complement_p)
{
  list * succs = &control_successors(c);
  
  pips_assert("We have at least one successor", gen_length(*succs)>0);
  
  if(gen_length(*succs)==1) {
    control c = CONTROL(CAR(*succs));
    
    pips_assert("The unique successor must be in partition",
		gen_in_list_p(c, partition)==!complement_p);
  }
  else if (gen_length(*succs)==2){
    /* Let's boldy assume the parent control node is associated to a test
       statement and preserve the number of successors. */
    MAPL(succ_c, 
    {
      control succ = CONTROL(CAR(succ_c));

      if(gen_in_list_p(succ, partition)==complement_p)
	CONTROL(CAR(succ_c)) = make_meaningless_control(CONS(CONTROL, c, NIL), NIL);
    }
	 , *succs);
  }
  else {
    /* It is not a test statement node: get rid of useless  */
    if(complement_p)
      gen_list_and_not(succs, partition);
    else
      gen_list_and(succs, partition);
  }
  pips_assert("We still have at least one successor", gen_length(*succs)>0);
}

void intersect_successors_with_partition(control c,
						       list partition)
{
  intersect_successors_with_partition_or_complement(c, partition, FALSE);
}

void intersect_successors_with_partition_complement(control c,
						    list partition)
{
  intersect_successors_with_partition_or_complement(c, partition, TRUE);
}

static void insert_non_deterministic_control_node(list succs,
						  control pred,
						  control new_c,
						  control old_c)
{
  if(FALSE) {
    /* Can we do without a extra-node? */
  }
  else{
    /* */  
  statement nop = make_continue_statement(entity_empty_label());
  control cnop = make_control(nop,
			      CONS(CONTROL, pred, NIL),
			      CONS(CONTROL, new_c, CONS(CONTROL, old_c, NIL)));
  pips_assert("succs points to old_c", CONTROL(CAR(succs))==old_c);
  pips_debug(2, "Allocate new control node %p as CONTINUE for test control node %p"
	     " with two successors, a new one, %p, and an old one %p\n",
	     cnop, pred, new_c, old_c);
  CONTROL(CAR(succs)) = cnop;
  gen_list_patch(control_predecessors(old_c), pred, cnop);
  gen_list_patch(control_predecessors(new_c), pred, cnop);
  }
  
}

static void update_successors_of_predecessor(control pred, control new_c, control old_c)
{
  /* These asserts might become wrong when intermediate CONTINUE nodes are
     used to carry many TRUE and/or FALSE successors */
  pips_assert("old_c is already a successor of pred",
	      gen_in_list_p(old_c, control_successors(pred)));
  pips_assert("new_c is not yet a successor of pred",
	      !gen_in_list_p(new_c, control_successors(pred)));
  
  if(statement_test_p(control_statement(pred))) {
    pips_assert("A test has exactly two successors",
		gen_length(control_successors(pred))==2);
    if(CONTROL(CAR(control_successors(pred)))==old_c) {
      insert_non_deterministic_control_node(control_successors(pred), pred, new_c, old_c);
    }
    else{
      insert_non_deterministic_control_node(CDR(control_successors(pred)), pred, new_c, old_c);
    }
  }
  else {
    /* No problem this kind of node may have several successors */
    control_successors(pred) = CONS(CONTROL, new_c, control_successors(pred));
  }
}

/* Make new_c a successor of new_pred, the same way c is a successor of
   pred */
static void update_successor_in_copy(control new_pred,
				     control pred,
				     control c,
				     control new_c)
{
  list succ_c = list_undefined;
  list new_succ_c = list_undefined;

  ifdebug(2) {
    pips_debug(2, "Begin for new_pred=%p, pred=%p, c=%p, new_c=%p\n",
	       new_pred, pred, c, new_c);
    print_control_node(new_pred);
    print_control_node(pred);
    print_control_node(c);
    print_control_node(new_c);
  }
  

  /* c and new_c have pred as predecessor */
  /* new_c must have new_pred as predecessor instead */
  pips_assert("c is a successor of pred",
	      gen_in_list_p(c, control_successors(pred)));
  pips_assert("pred is a predecessor of new_c",
	      gen_in_list_p(pred, control_predecessors(new_c)));

  /* Scan together both successor lists of the two predecessors to update
     the successor of the new predecessor */
  pips_assert("The two successor lists have same length",
	      gen_length(control_successors(pred))
	      == gen_length(control_successors(new_pred)));
  for(succ_c = control_successors(pred), new_succ_c = control_successors(new_pred);
      !ENDP(succ_c) && !ENDP(new_succ_c); POP(succ_c), POP(new_succ_c)){
    control succ = CONTROL(CAR(succ_c));
    if(succ==c) {
      control new_succ = CONTROL(CAR(new_succ_c));
      if(meaningless_control_p(new_succ)){
	pips_debug(2, "Free meaningless control node %p\n", new_succ);
	free_meaningless_control(new_succ);
	CONTROL(CAR(new_succ_c)) = new_c;
      }
      else{
	/* insert non-deterministic node */
	pips_internal_error("Not implemented yet\n");
      }
      break;
    }
  }
  pips_assert("The leftover two successor lists still have same length",
	      gen_length(succ_c) == gen_length(new_succ_c));

  ifdebug(2) {
    pips_debug(2, "End with new_pred=%p, pred=%p, c=%p, new_c=%p\n",
	       new_pred, pred, c, new_c);
    print_control_node(new_pred);
    print_control_node(pred);
    print_control_node(c);
    print_control_node(new_c);
  }
}

static void shallow_free_control(control c)
{
  /* free_control() cannot be used because you do not want to free the
     statement, the successors and the predecessors. But you want to get
     rid of meaningless control nodes */
  control_statement(c) = statement_undefined;

  MAP(CONTROL, c, 
  {
    if(meaningless_control_p(c)) {
      free_meaningless_control(c);
    }
  }
      , control_successors(c));
  
  gen_free_list(control_predecessors(c));
  gen_free_list(control_successors(c));
  control_predecessors(c)= list_undefined;
  control_successors(c)= list_undefined;

  free_control(c);
}

/* The nodes in scc partition but root must be removed. Replicated nodes
   must be added to build all input and output paths to and from root
   using control nodes in partition. */

static unstructured scc_to_dag(control root, list partition, hash_table ancestor_map)
{
  unstructured u = unstructured_undefined;
  control new_root = control_undefined;
  hash_table replicated_input_controls = hash_table_make(hash_pointer, 0);
  hash_table replicated_output_controls = hash_table_make(hash_pointer, 0);
  bool stable_graph_p = FALSE;
  int number_in = 0;
  int number_out = 0;
  int iteration = 0;
  
  ifdebug(2) {
    pips_debug(2, "Begin for vertex %p and partition:\n", root);
    print_control_nodes(partition);
  }

  while(!stable_graph_p) {
    int number = 0;
    stable_graph_p = TRUE;
    iteration++;
    
    /* Look for new input nodes in partition */
    MAP(CONTROL, c, 
    {
      if(c!=root && hash_get(replicated_input_controls, c)==HASH_UNDEFINED_VALUE) {
	if(entry_control_p(c, partition)) {
	  control new_c1 = control_undefined;
	  
	  /* c cannot have been replicated earlier or it would not be in partition */
	  pips_assert("c has not yet been input replicated",
		      hash_get(replicated_input_controls, c)==HASH_UNDEFINED_VALUE);
	  new_c1 = make_control(control_statement(c),
				gen_copy_seq(control_predecessors(c)),
				gen_copy_seq(control_successors(c)));
	  add_child_parent_pair(ancestor_map, new_c1, c);

	  pips_debug(2, "Allocate new input control node new_c1=%p as a copy of node %p\n",
		new_c1, c);
	  
	  /* Keep only predecessors out of partition for new_c1. */
	  gen_list_and_not(&control_predecessors(new_c1), partition);
	  /* Make sure that new_c1 is a successor of its
             predecessors... Oupss, there is a problem with test to encode
             the fact that is is a true or a false successor? */
	  MAP(CONTROL, pred_c1, {
	    gen_list_patch(control_successors(pred_c1), c, new_c1);
	  },control_predecessors(new_c1) );
	  
	  
	  /* Keep only c's predecessors in partition. 

	     That's probably wrong and damaging because we need them for
	     paths crossing directly the SCC and useless because these
	     nodes will be destroyed in parent graph. */
	  /* gen_list_and(&control_predecessors(c), partition); */
	  /* Update the successor of its predecessors */
	  
	  /* Update successors of new_c1 which have already been replicated */
	  MAPL(succ_c,
	  {
	    control succ = CONTROL(CAR(succ_c));
	    control new_succ = hash_get(replicated_input_controls, succ);
	    if(new_succ!=HASH_UNDEFINED_VALUE) {
	      CONTROL(CAR(succ_c)) = new_succ;
	      /* Add new_c1 as predecessor of new_succ */
	      control_predecessors(new_succ) =
		gen_once(new_c1, control_predecessors(new_succ));
	    }
	  }
	       , control_successors(new_c1));

	  stable_graph_p = FALSE;
	  number++;
	  number_in++;
	  hash_put(replicated_input_controls, c, new_c1);
	}
      }
    }
	, partition);

    ifdebug(2) {
      if(number>0) {
	pips_debug(2, "Embedding graph after replication"
		   " of %d input control node(s) at iteration %d:\n",
		   number, iteration);
	print_embedding_graph(root);
	pips_debug(2, "End of embedding graph after replication"
		   " of input control nodes\n");
      }
      else {
	pips_debug(2, "No new input control node at iteration %d\n",
		   iteration);
      }
    }
    
    /* Look for new output nodes */
    number = 0;
    MAP(CONTROL, c, 
    {
      if(c!=root && hash_get(replicated_output_controls, c)==HASH_UNDEFINED_VALUE) {
	if(exit_control_p(c, partition)) {
	  control new_c2 = control_undefined;
	  
	  /* c cannot have been replicated earlier or it would not be in partition */
	  pips_assert("c has not yet been output replicated",
		      hash_get(replicated_output_controls, c)==HASH_UNDEFINED_VALUE);
	  new_c2 = make_control(control_statement(c),
				gen_copy_seq(control_predecessors(c)),
				gen_copy_seq(control_successors(c)));
	  add_child_parent_pair(ancestor_map, new_c2, c);

	  pips_debug(2, "Allocate new output control node new_c2=%p as a copy of node %p\n",
		new_c2, c);

	  /* Keep only successors out of partition for new_c2. */
	  /* A true branch successor may have to be replaced to preserve
             the position of the false branch successor. */
	  /* gen_list_and_not(&control_successors(new_c2), partition); */
	  intersect_successors_with_partition_complement(new_c2, partition);
	  
	  /* Update reverse arcs */
	  MAP(CONTROL, succ_c2, {
	    gen_list_patch(control_predecessors(succ_c2), c, new_c2);
	  }, control_successors(new_c2) );
	  
	  /* Keep only c's succcessors in partition. Wise or not? See above... */
	  /* gen_list_and(&control_successors(c), partition); */
	  intersect_successors_with_partition(c, partition);
	  /* Reverse arcs have already been correctly updated */
	
	  /* Update predecessors of new_c2 which have already been replicated */
	  MAPL(pred_new_c2_c,
	  {
	    control pred_new_c2 = CONTROL(CAR(pred_new_c2_c));
	    control new_pred_new_c2 = hash_get(replicated_output_controls, pred_new_c2);
	    if(new_pred_new_c2!=HASH_UNDEFINED_VALUE) {
	      /* gen_list_patch() has no effect because new_pred's
                 successors have already been updated and c does not
                 appear there. */
	      /* gen_list_patch(control_successors(new_pred), c, new_c2); */
	      update_successor_in_copy(new_pred_new_c2, pred_new_c2, c, new_c2);
	      CONTROL(CAR(pred_new_c2_c)) = new_pred_new_c2;
	    }
	    else{
	      /* Update reverse edges of predecessors... which may be
		 tough if you end up with more than one TRUE and/or more
		 than one FALSE successor. */
	      update_successors_of_predecessor(pred_new_c2, new_c2, c);
	    }
	  }
	       , control_predecessors(new_c2));

	  stable_graph_p = FALSE;
	  number++;
	  number_out++;
	  hash_put(replicated_output_controls, c, new_c2);
	}
      }
    }
	, partition);

    ifdebug(2) {
      if(number>0) {
	pips_debug(2, "Embedding graph after replication"
		   " of output control nodes at iteration %d:\n", iteration);
	print_embedding_graph(root);
	pips_debug(2, "End of embedding graph after replication"
		   " of output control nodes\n");
      }
      else {
	pips_debug(2, "No new input control node at iteration %d\n", iteration);
      }
    }

  }

  ifdebug(2) {
    if(number_out>0) {
      pips_assert("At least two iterations", iteration>1);
      pips_debug(2, "Final embedding graph after replication of all input and output paths (%d iterations)\n", iteration);
      print_embedding_graph(root);
      pips_debug(2, "End of final embedding graph after replication of all input and output paths\n");
    }
    else {
      pips_assert("Only one iteration", iteration==1);
      pips_debug(2, "No new output paths\n");
    }
  }

  /* Remove cycle partition but its head */
  /* We do not want to free these nodes because they are part of
     partition.  We should not have duplicated partition earlier to make a
     new unstructured.  We only need a copy of the head, root. This should
     be dealt with after scc_to_dag is called and not before.

  MAP(CONTROL, c, 
  {
    if(c!=root) {
      shallow_free_control(c);
    }
  }
      , partition); */
  /* Unlink partition from its head.
     We could keep an arc from root to root... */
  /* This unlinking would better be performed at the caller level, even
     though this would make the name scc_to_dag() imprecise. */

  /* replicate root and unlink the cycles defined by partition from the
     embedding graph */
  new_root = make_control(control_statement(root),
		       gen_copy_seq(control_predecessors(root)),
		       gen_copy_seq(control_successors(root)));

  add_child_parent_pair(ancestor_map, new_root, root);
  u = make_unstructured(new_root, new_root);
  gen_list_and_not(&control_predecessors(root), partition);
  intersect_successors_with_partition_complement(root, partition);
  gen_list_and(&control_predecessors(new_root), partition);
  intersect_successors_with_partition(new_root, partition);
  MAP(CONTROL, c, 
  {
    gen_list_patch(control_successors(c), root, new_root);
    gen_list_patch(control_predecessors(c), root, new_root);
  }
      , partition);
  
  ifdebug(2) {
    list l_root = node_to_linked_nodes(root);
    list l_new_root = node_to_linked_nodes(new_root);
    
    pips_debug(2, "Nodes linked to root=%p\n", root);
    print_control_node(root);
    print_control_nodes(l_root);
    pips_debug(2, "Nodes linked to new_root=%p\n", new_root);
    print_control_node(new_root);
    print_control_nodes(l_new_root);
    
    gen_list_and(&l_root, l_new_root);
    
    if(!ENDP(l_root)) {
      fprintf(stderr,
	      "Bug: l_root and l_new_root share at least one node and must be equal\n");
      pips_assert("l_root and l_new_root have an empty intersection",
		  ENDP(l_root));
    }
    
    pips_debug(2, "Final embedding graph after replication and cycle removal\n");
    print_embedding_graph(root);
    pips_debug(2, "End of final embedding graph after replication and cycle removal\n");
    pips_debug(2, "Final cycle graph\n");
    print_embedding_graph(new_root);
    pips_debug(2, "End of final cycle graph\n");

    /* All nodes are linked to an ancestor node or are an ancestor or are
       meaningless */
    pips_assert("All nodes in l_root are registered",
		registered_controls_p(ancestor_map, l_root));
    pips_assert("All nodes in l_new_root are registered",
		registered_controls_p(ancestor_map, l_new_root));
    pips_assert("Children and ancestors have an empty intersection",
		ancestor_map_consistent_p(ancestor_map));
    
    gen_free_list(l_root);
    gen_free_list(l_new_root);
  }
   
  ifdebug(2) {
    pips_debug(2, "End with %d replicated control nodes:\n\n", number_in+number_out);
    if(number_in>0){
      print_control_to_control_mapping("replicated_input_controls",
				       replicated_input_controls);
    }
    
    if(number_out>0) {
      print_control_to_control_mapping("replicated_output_controls",
				       replicated_output_controls);
    }
    
  }

  hash_table_free(replicated_input_controls);
  hash_table_free(replicated_output_controls);

  return u;
}


/* Decomposition of control flow graph u into a DAG new_u and two
   mappings. Mapping scc_map maps nodes of u used to break cycles to the
   unstructured representing these cycles. Mapping ancestor_map maps nodes
   used in DAG new_u or in unstructured refered to by scc_mapp to nodes in
   u. */
unstructured bourdoncle_partition(unstructured u,
				  hash_table *p_ancestor_map,
				  hash_table * p_scc_map)
{
  list partition = NIL;
  control root = control_undefined;
  /* DAG derived from u by elimination all cycles */
  unstructured new_u = unstructured_undefined;
  /* mapping from nodes in the new unstructured to the node in the input unstructured */
  hash_table ancestor_map = hash_table_make(hash_pointer, 0);
  /* mapping from nodes in u, used as cycle breakers, to the corresponding
     scc represented as an unstructured */
  hash_table scc_map = hash_table_make(hash_pointer, 0);
  
  ifdebug(2) {
    pips_debug(2, "Begin with unstructured:\n");
    print_unstructured(u);
  }

  make_vertex_stack();
  num = 0;
  dfn = hash_table_make(hash_pointer, 0);

  new_u = unstructured_shallow_copy(u, ancestor_map);

  ifdebug(2) {
    pips_debug(2, "Copied unstructured new_u:\n");
    print_unstructured(new_u);
  }
      

  /* Initialize dfn to 0 */
  gen_multi_recurse(new_u, statement_domain, gen_false, gen_null,
		    control_domain, gen_true, reset_dfn, NULL);

  /* Start from the entry point */
  root = unstructured_control(new_u);  
  (void) bourdoncle_visit(root, &partition, ancestor_map, scc_map);
    
  free_vertex_stack();
  hash_table_free(dfn);

  ifdebug(2) {
    pips_debug(2, "End with partition:");
    print_control_nodes(partition);
  }

  *p_ancestor_map = ancestor_map;
  *p_scc_map = scc_map;

  return new_u;
}

static void update_partition(control vertex,
			     list partition,
			     hash_table ancestor_map)
{
  /* Controls in partition may have been copied and may not appear anymore
     in the graph embedding vertex. The input graph is modified to
     separate clean cycles with one entry-exit node, but Bourdoncle's
     algorithm is not aware of this.  */
  list embedding_nodes = node_to_linked_nodes(vertex);
  int changes = 0;

  pips_assert("The head is in the partition", gen_in_list_p(vertex, partition));

  ifdebug(2) {
    pips_debug(2, "Begin for partition:\n");
    print_control_nodes(partition);
  }
  
  MAPL(c_c,
  {
    control c = CONTROL(CAR(c_c));
    pips_assert("c is not a meaningless control node",
		!meaningless_control_p(c));
    
    if(!gen_in_list_p(c, embedding_nodes)) {
      /* Find a replacement for c, and hope to find only one! */
      control a = control_to_ancestor(c, ancestor_map);
      list replacement_list = NIL;

      changes++;

      MAP(CONTROL, c_new, 
      {
	control a_new = control_to_ancestor(c_new, ancestor_map);
	if(a==a_new) {
	  replacement_list = CONS(CONTROL, c_new, replacement_list);
	}
      }
	  , embedding_nodes);

      switch(gen_length(replacement_list)) {
      case 0: 
	pips_internal_error("No replacement for node %p\n", c);
	break;
      case 1: CONTROL(CAR(c_c)) = CONTROL(CAR(replacement_list));
	break;
      default: fprintf(stderr,
		       "Too many replacement nodes (%d) in embedding nodes for node %p\n",
		       gen_length(replacement_list), c);
	pips_internal_error("Too many replacement nodes\n");
	break;
      }
      
      gen_free_list(replacement_list);
    }
  }
       , partition);

  ifdebug(2) {
    if(changes==0) {
      pips_debug(2, "End with same partition\n");
    }
    else{
      pips_debug(2, "End with new partition:\n");
      print_control_nodes(partition);
    }
  }
}


list bourdoncle_component(control vertex,
			  hash_table ancestor_map,
			  hash_table scc_map)
{
  list partition = NIL;
  unstructured u = unstructured_undefined;
  control vertex_ancestor = control_undefined;
  
  ifdebug(2) {
    pips_debug(2, "Begin for node: \n");
    print_control_node(vertex);
  }

  MAP(CONTROL, c, 
  {
    if(get_dfn(c)==0) {
      (void) bourdoncle_visit(c, &partition, ancestor_map, scc_map);
    }
  }
      , control_successors(vertex));
  
  partition = CONS(CONTROL, vertex, partition);
  
  /* Build sub-unstructured associated to vertex and partition */
  /* Re-use copying performed in scc_to_dag() instead
  u = partition_to_unstructured(vertex, partition);
  vertex_ancestor = control_to_ancestor(vertex, ancestor_map);
  hash_put(scc_map, vertex_ancestor, u);
  */

  /* The partition may have to be refreshed because of the previous
     recursive descent and its graph restructuring action. It is assumed
     that vertex is still good. */
  update_partition(vertex, partition, ancestor_map);
  
  /* Update parent unstructured containing vertex and partition, remove
     partition and return a new unstructured with the partition inside,
     except for the initial vertex node which is left in the embedding
     graph */
  u = scc_to_dag(vertex, partition, ancestor_map);

  /* Build sub-unstructured associated to vertex and partition */
  /* u = unlink_partition_and build_unstructured(vertex, partition); */
  vertex_ancestor = control_to_ancestor(vertex, ancestor_map);
  hash_put(scc_map, vertex_ancestor, u);

  ifdebug(2) {
    pips_debug(2, "End with partition: ");
    print_control_nodes(partition);
    pips_debug(2, "End with new nodes:\n");
    /* Do not go down into nested unstructured */
    gen_multi_recurse(u, statement_domain, gen_false, gen_null,
		      control_domain, gen_true, print_control_node, NULL);
    pips_debug(2, "With entry nodes\n");
    print_control_node(unstructured_control(u));
    pips_debug(2, "And exit node\n");
    print_control_node(unstructured_exit(u));
    pips_debug(2, "End.\n");
  }

  return partition;
}


int bourdoncle_visit(control vertex,
		     list * ppartition,
		     hash_table ancestor_map,
		     hash_table scc_map)
{
  int min = 0;
  int head = 0;
  bool loop = FALSE;
  
  vertex_push(vertex);
  num = num+1;
  head = num;
  update_dfn(vertex, num);
  
  MAP(CONTROL, succ, 
  {
    if(get_dfn(succ)==0) {
      min = bourdoncle_visit(succ, ppartition, ancestor_map, scc_map);
    }
    else {
      min = get_dfn(succ);
    }
    if(min<=head) {
      head = min;
      loop = TRUE;
    }
  }
      , control_successors(vertex));

  if (head==get_dfn(vertex)) {
    control e = vertex_pop();

    update_dfn(vertex, LONG_MAX);

    if(loop) {
      while(e!=vertex) {
	update_dfn(e, 0);
	e = vertex_pop();
      }
      *ppartition = gen_nconc(bourdoncle_component(vertex, ancestor_map, scc_map),
			      *ppartition);
    }
    else {
      *ppartition = CONS(CONTROL, vertex, *ppartition);
    }

  }
  
  return head;
}
