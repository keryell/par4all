/* HPFC - Fabien Coelho, May 1993 and later...
 *
 * $RCSfile: compiler.c,v $ ($Date: 1996/06/12 15:52:46 $, )
 * version $Revision$
 *
 * Compiler
 *
 * stat is the current statement to be compiled, and there are
 * pointers to the current statement building of the node and host
 * codes. the module of these are also kept in order to add
 * the needed declarations generated by the compilation.
 *
 * however, every entities of the compiled program, and of
 * both generated programs will be mixed, due to the
 * tabulated nature of these objects. 
 * some objects will be shared.
 * I don't think this is a problem.
 */

#include "defines-local.h"

#include "control.h"     /* for CONTROL_MAP() */

/* global variables
 */
entity host_module, node_module;

#define debug_print_control(c, w)\
  fprintf(stderr, \
	  "%s: ctr 0x%x (stat 0x%x) , %d preds, %d succs\n", w, \
          (unsigned int) c, (unsigned int) control_statement(c), \
	  gen_length(control_predecessors(c)), \
	  gen_length(control_successors(c))); \
  print_statement(control_statement(c));

static void 
hpf_compile_block(stat, hoststatp, nodestatp)
statement stat;
statement *hoststatp,*nodestatp;
{
    list /* of statements */ lhost=NIL, lnode=NIL;
    statement hostcd, nodecd;

    pips_assert("block", instruction_block_p(statement_instruction(stat)));

    (*hoststatp) = MakeStatementLike(stat, is_instruction_block);
    (*nodestatp) = MakeStatementLike(stat, is_instruction_block);

    MAP(STATEMENT, s,
     {
	 hpf_compiler(s,&hostcd,&nodecd);
	 
	 lhost = CONS(STATEMENT, hostcd, lhost);
	 lnode = CONS(STATEMENT, nodecd, lnode);
     },
	 instruction_block(statement_instruction(stat)));

    instruction_block(statement_instruction(*hoststatp)) = gen_nreverse(lhost);
    instruction_block(statement_instruction(*nodestatp)) = gen_nreverse(lnode);

    DEBUG_STAT(9, entity_name(host_module), *hoststatp);
    DEBUG_STAT(9, entity_name(node_module), *nodestatp);
}

static void 
hpf_compile_test(s, hoststatp, nodestatp)
statement s;
statement *hoststatp,*nodestatp;
{
    statement
	s_true,	s_hosttrue, s_nodetrue,
	s_false, s_hostfalse, s_nodefalse;
    test the_test;
    expression condition;
    
    pips_assert("test", instruction_test_p(statement_instruction(s)));

    the_test = instruction_test(statement_instruction(s));
    condition = test_condition(the_test);
    
    (*hoststatp) = MakeStatementLike(s, is_instruction_test);
    (*nodestatp) = MakeStatementLike(s, is_instruction_test);

    /* if it may happen that a condition modifies the value
     * of a distributed variable, this condition is to be
     * put out of the statement, for separate compilation.
     */

    s_true = test_true(the_test);
    s_false = test_false(the_test);

    hpf_compiler(s_true, &s_hosttrue, &s_nodetrue);
    hpf_compiler(s_false, &s_hostfalse, &s_nodefalse);

    instruction_test(statement_instruction(*hoststatp)) =
	make_test(UpdateExpressionForModule(host_module, condition),
		  s_hosttrue,
		  s_hostfalse);

    instruction_test(statement_instruction(*nodestatp))=
	make_test(UpdateExpressionForModule(node_module, condition),
		  s_nodetrue,
		  s_nodefalse);

    DEBUG_STAT(9, entity_name(host_module), *hoststatp);
    DEBUG_STAT(9, entity_name(node_module), *nodestatp);
}

/* return the list of bounds
 */
static list /* of expression */ 
caller_list_of_bounds(
    entity fun,
    list /* of expression */ le)
{
    list /* of expression */ lneeded = NIL;
    int len = gen_length(le);

    for (; len>=1; len--)
    {
	expression e = EXPRESSION(gen_nth(len-1, le));
	syntax s = expression_syntax(e);

	if (syntax_reference_p(s)) 
	{
	    entity var, old;

	    var = reference_variable(syntax_reference(s));
	    old = load_old_node(var);
	    pips_debug(8, "considering %s\n", entity_name(var));

	    if (array_distributed_p(old))
	    {
		int dim = NumberOfDimension(var);

		for (; dim>=1; dim--)
		{
		    if (ith_dim_overlapable_p(old, dim))
		    {
			lneeded = 
			    CONS(EXPRESSION, hpfc_array_bound(var, FALSE, dim),
			    CONS(EXPRESSION, hpfc_array_bound(var, TRUE, dim),
				 lneeded));
		    }
		}
	    }
	}
    }

    return lneeded;
}

static void 
hpf_compile_call(
    statement stat,       /* compiled statement */
    statement *hoststatp, /* returned host version */
    statement *nodestatp) /* returned node version */
{
    call c = instruction_call(statement_instruction(stat));

    /* IO functions should be detected earlier, in hpf_compiler
     */
    pips_assert("not an io call", !IO_CALL_P(c));
    pips_assert("call", instruction_call_p(statement_instruction(stat)));
    pips_debug(7, "function %s\n", entity_name(call_function(c)));

    DEBUG_STAT(9, "statement", stat);

    /* "kill" FC directive.
     * tells that the array is dead, hence all copies are live...
     */
    if (dead_fcd_directive_p(call_function(c)))
    {
	list /* of statement */ ls = NIL;

	MAP(EXPRESSION, e,
	{
	    entity primary = expression_to_entity(e);
	    pips_debug(5, "dealing with array %s\n", entity_name(primary));
	    if (array_distributed_p(primary))
		ls = CONS(STATEMENT, generate_all_live(primary), ls);
	},
	    call_arguments(c));
	
	(*hoststatp) = MakeStatementLike(stat, is_instruction_block);
	(*nodestatp) = MakeStatementLike(stat, is_instruction_block);
	
	instruction_block(statement_instruction(*hoststatp)) = NIL;
	instruction_block(statement_instruction(*nodestatp)) = ls;

	return;
    }

    /* no reference to distributed arrays...
     * the call is just translated into local objects.
     */
    if (!ref_to_dist_array_p(c))
    {
	pips_debug(7, "no reference to distributed variable\n");

	(*hoststatp)=MakeStatementLike(stat, is_instruction_call);
	(*nodestatp)=MakeStatementLike(stat, is_instruction_call);
	
	instruction_call(statement_instruction((*hoststatp))) = copy_call(c);
	update_object_for_module(*hoststatp, host_module);

	instruction_call(statement_instruction((*nodestatp))) = copy_call(c);
	update_object_for_module(*nodestatp, node_module);

	DEBUG_STAT(8, entity_name(host_module), *hoststatp);
	DEBUG_STAT(8, entity_name(node_module), *nodestatp);

	return;
    }

    /* should consider read and written variables
     */    
    if (ENTITY_ASSIGN_P(call_function(c)))
    {
	list /* of expressions */
	    lh = NIL, ln = NIL, args = call_arguments(c) ;
	expression 
	    w = EXPRESSION(CAR(args)),
	    r = EXPRESSION(CAR(CDR(args)));

	pips_assert("reference", syntax_reference_p(expression_syntax(w)));

	if (array_distributed_p
	    (reference_variable(syntax_reference(expression_syntax(w)))))
	{
	    pips_debug(8, "c1-alpha\n");
	    
	    generate_c1_alpha(stat, &lh, &ln); /* C1-ALPHA */
	}
	else
	{
	    syntax s = expression_syntax(r);

	    /* reductions are detected here. They are not handled otherwise
	     */
	    if (syntax_call_p(s) && call_reduction_p(syntax_call(s)))
	    {
		statement sh, sn;

		if (!compile_reduction(stat, &sh, &sn))
		    pips_internal_error("reduction compilation failed\n");

		lh = CONS(STATEMENT, sh, NIL);
		ln = CONS(STATEMENT, sn, NIL);
	    }
	    else
	    {
		pips_debug(8, "c1-beta\n");
		
		generate_c1_beta(stat, &lh, &ln); /* C1-BETA */
	    }
	}

	(*hoststatp) = MakeStatementLike(stat, is_instruction_block);
	(*nodestatp) = MakeStatementLike(stat, is_instruction_block);
	
	instruction_block(statement_instruction(*hoststatp)) = lh;
	instruction_block(statement_instruction(*nodestatp)) = ln;
	
	DEBUG_STAT(8, entity_name(host_module), *hoststatp);
	DEBUG_STAT(8, entity_name(node_module), *nodestatp);

	return;
    }

    /* call to something with distributed variables, which is not an
     * assignment. Since I do not use the effects as I should, nothing is
     * done...
     */

    /* temporary (?:-) hack 
     */
    {
	entity fun = call_function(c);
	list /* of expressions */
            args = call_arguments(c),
	    leh=lUpdateExpr_but_distributed(host_module, args),
	    len=lUpdateExpr(node_module, args);
	
	update_overlaps_in_caller(fun, args);

	pips_debug(7, "some references to distributed variable\n");

	(*hoststatp)=MakeStatementLike(stat, is_instruction_call);
	(*nodestatp)=MakeStatementLike(stat, is_instruction_call);
	
	instruction_call(statement_instruction((*hoststatp)))=
	    make_call(fun, leh);

	instruction_call(statement_instruction((*nodestatp)))=
	    make_call(fun, gen_nconc(len, caller_list_of_bounds(fun, args)));

	DEBUG_STAT(8, entity_name(host_module), *hoststatp);
	DEBUG_STAT(8, entity_name(node_module), *nodestatp);

	return;
    }
}

static void compile_control(
    control c,
    statement_mapping maph,
    statement_mapping mapn)
{
    control hostc, nodec;
    statement stath, statn, statc = control_statement(c);
    
    hpf_compiler(statc, &stath, &statn);
    
    DEBUG_STAT(7, "statc", statc);
    DEBUG_STAT(7, "host stat", stath);
    DEBUG_STAT(7, "node stat", statn);
    
    hostc = make_control(stath, NIL, NIL);
    SET_CONTROL_MAPPING(maph, c, hostc);
    
    nodec = make_control(statn, NIL, NIL);
    SET_CONTROL_MAPPING(mapn, c, nodec);
}

static void 
hpf_compile_unstructured(
    statement stat,
    statement *hoststatp,
    statement *nodestatp)
{
    instruction inst=statement_instruction(stat);

    pips_assert("unstructured", instruction_unstructured_p(inst));

    if (one_statement_unstructured(instruction_unstructured(inst)))
    {
	pips_debug(7, "one statement recognize\n");

	/* nothing spacial is done! 
	 * ??? there may be a problem with the label of the statement, if any.
	 */
	hpf_compiler(control_statement
	     (unstructured_control(instruction_unstructured(inst))),
	     hoststatp, nodestatp);
    }
    else
    {
	control_mapping 
	    hostmap = MAKE_CONTROL_MAPPING(),
	    nodemap = MAKE_CONTROL_MAPPING();
	unstructured  u = instruction_unstructured(inst);
	control ct = unstructured_control(u),
	        ce = unstructured_exit(u), new_ct, new_ce;
	list blocks = NIL;

	pips_debug(6, "beginning\n");

	CONTROL_MAP(c, compile_control(c, hostmap, nodemap), ct, blocks);

	if (!gen_in_list_p(ce, blocks))
	{
	    pips_debug(5, "exit not in blocks\n");
	    blocks = CONS(CONTROL, ce, blocks);
	    compile_control(ce, hostmap, nodemap);
	}
	
	MAP(CONTROL, c,
	 {
	     update_control_lists(c, hostmap);
	     update_control_lists(c, nodemap);
	 },
	    blocks);

	ifdebug(9)
	{
	    control h_tmp, n_tmp;

	    pips_debug(9, "controls:\n");
	    
	    MAP(CONTROL, c_tmp,
	     {
		 h_tmp = (control) GET_CONTROL_MAPPING(hostmap, c_tmp);
		 n_tmp = (control) GET_CONTROL_MAPPING(nodemap, c_tmp);
		 
		 debug_print_control(c_tmp, "initial");
		 debug_print_control(h_tmp, "host");
		 debug_print_control(n_tmp, "node");
	     },
		 blocks);
	}

	/*    HOST statement
	 */
	(*hoststatp) = MakeStatementLike(stat, is_instruction_unstructured);

	new_ct = (control) GET_CONTROL_MAPPING(hostmap, ct);
	new_ce = (control) GET_CONTROL_MAPPING(hostmap, ce);

	pips_assert("defined control", !control_undefined_p(new_ct) &&
		    !control_undefined_p(new_ce));

	ifdebug(9)
	{
	    pips_debug(9, "host controls for [0x%x,0x%x]:\n", 
		       (unsigned int) ct, (unsigned int) ce);
	    
	    debug_print_control(new_ct, "main");
	    debug_print_control(new_ce, "exit");
	}

	instruction_unstructured(statement_instruction(*hoststatp)) =
	    make_unstructured(new_ct, new_ce);

	DEBUG_STAT(7, "host new stat", *hoststatp);

	/*    NODE statement
	 */
	(*nodestatp) = MakeStatementLike(stat, is_instruction_unstructured);

	new_ct = (control) GET_CONTROL_MAPPING(nodemap, ct);
	new_ce = (control) GET_CONTROL_MAPPING(nodemap, ce);

	pips_assert("defined control",
		    !control_undefined_p(new_ct) && 
		    !control_undefined_p(new_ce));

	instruction_unstructured(statement_instruction(*nodestatp)) =
	    make_unstructured(new_ct, new_ce);

	DEBUG_STAT(7, "host new stat (again)", *hoststatp);
	
	gen_free_list(blocks);
	FREE_CONTROL_MAPPING(hostmap);
	FREE_CONTROL_MAPPING(nodemap);
    }
}

static void 
hpf_compile_sequential_loop(stat,hoststatp,nodestatp)
statement stat, *hoststatp, *nodestatp;
{
    loop the_loop=statement_loop(stat);
    statement body = loop_body(the_loop), hostbody, nodebody;
    range r=loop_range(the_loop);
    list /* of entities */ locals=loop_locals(the_loop);
    entity
	label = loop_label(the_loop),
	index = loop_index(the_loop),
	nindex = NewVariableForModule(node_module, index),
	hindex = NewVariableForModule(host_module, index);
    expression
	lower = range_lower(r),
	upper = range_upper(r),
	increment = range_increment(r);
    
    hpf_compiler(body, &hostbody, &nodebody);
    
    if (hpfc_empty_statement_p(hostbody))
    {
	/* ??? memory leak, hostbody is lost whatever it was.
	 */
	(*hoststatp)=make_continue_statement(entity_undefined);
    }
    else
    {
	(*hoststatp)=MakeStatementLike(stat, is_instruction_loop);

	instruction_loop(statement_instruction(*hoststatp))=
	  make_loop(hindex,
		    make_range(UpdateExpressionForModule(host_module,lower),
			       UpdateExpressionForModule(host_module,upper),
			    UpdateExpressionForModule(host_module,increment)),
		    hostbody,
		    label,
		    make_execution(is_execution_sequential,UU),
		    lNewVariableForModule(host_module,locals));
    }

    DEBUG_STAT(8, "host stat", *hoststatp);

    (*nodestatp)=MakeStatementLike(stat, is_instruction_loop);

    instruction_loop(statement_instruction(*nodestatp))=
	make_loop(nindex,
		  make_range(UpdateExpressionForModule(node_module,lower),
			     UpdateExpressionForModule(node_module,upper),
			     UpdateExpressionForModule(node_module,increment)),
		  nodebody,
		  label,
		  make_execution(is_execution_sequential,UU),
		  lNewVariableForModule(node_module,locals));

    DEBUG_STAT(8, "node stat", *nodestatp);
}

static void 
hpf_compile_parallel_body(body, hoststatp, nodestatp)
statement body, *hoststatp, *nodestatp;
{
    list lw = NIL, lr = NIL, li = NIL, ls = NIL, lbs = NIL;

    /* ???
     * dependances are not surely respected in the definitions list...
     * should check that only locals variables, that are not replicated,
     * may be defined during the body of the loop...
     */
    FindRefToDistArrayInStatement(body, &lw, &lr);
    li = AddOnceToIndicesList(lIndicesOfRef(lw), lIndicesOfRef(lr));
    ls = FindDefinitionsOf(body, li);
    gen_free_list(li), li=NIL;

    if (gen_length(lw)==0 && gen_length(lr)==0) /* very partial */
    {
	(*hoststatp) = copy_statement(body);
	(*nodestatp) = copy_statement(body);
    }
    else
    {
	generate_parallel_body(body, &lbs, lw, lr);
	
	(*hoststatp) = NULL;
	(*nodestatp) = make_block_statement(gen_nconc(ls, lbs));
    }

    gen_free_list(lw), lw=NIL;
    gen_free_list(lr), lr=NIL;
}

static void hpf_compile_parallel_loop(stat, hoststatp, nodestatp)
statement stat, *hoststatp, *nodestatp;
{
    loop the_loop = statement_loop(stat);
    statement s, nodebody, body = loop_body(the_loop);
    instruction bodyinst = statement_instruction(body);
    entity
	label = loop_label(the_loop),
	index = loop_index(the_loop),
	nindex = NewVariableForModule(node_module,index);
    range r = loop_range(the_loop);
    expression
	lower = range_lower(r),
	upper = range_upper(r),
	increment = range_increment(r);
    
    pips_assert("parallel loop",
		execution_parallel_p(loop_execution(the_loop)));

    if ((instruction_loop_p(bodyinst)) &&
	(execution_parallel_p(loop_execution(instruction_loop(bodyinst)))))
	hpf_compile_parallel_loop(body, &s, &nodebody);
    else
	hpf_compile_parallel_body(body, &s, &nodebody);
    
    (*hoststatp) = make_continue_statement(entity_undefined);
    (*nodestatp) = MakeStatementLike(stat, is_instruction_loop);

    instruction_loop(statement_instruction(*nodestatp))=
	make_loop(nindex,
		  make_range(UpdateExpressionForModule(node_module,lower),
			     UpdateExpressionForModule(node_module,upper),
			     UpdateExpressionForModule(node_module,increment)),
		  nodebody,
		  label,
		  make_execution(is_execution_sequential,UU),
		  NULL);
}

static void 
hpf_compile_loop(stat, hoststatp, nodestatp)
statement stat;
statement *hoststatp, *nodestatp;
{
    loop the_loop = instruction_loop(statement_instruction(stat));

    pips_assert("loop", statement_loop_p(stat));

    if (execution_parallel_p(loop_execution(the_loop)))
    {
	entity var;
	reference left, right;
	list l=NIL;
	bool is_shift = subarray_shift_p(stat, &var, &l),
	     is_full_copy = full_copy_p(stat, &left, &right),
	    /* 
	     * should verify that only listed in labels and distributed
	     * entities are defined inside the body of the loop
	     */
	    at_ac = atomic_accesses_only_p(stat),
	    in_in = indirections_inside_statement_p(stat);
	
	debug(5, "hpf_compile_loop", "condition results: sh %d, aa %d, in %d\n",
	      is_shift, at_ac, in_in);

	if (is_full_copy)
	{
	    pips_debug(4, "full copy detected\n");

	    *nodestatp = generate_full_copy(left, right);
	    *hoststatp = make_empty_statement();
	}
	else if (is_shift)
	{
	    pips_debug(4, "shift detected\n");
	    
	    *nodestatp = generate_subarray_shift(stat, var, l);
	    *hoststatp = make_empty_statement();
	}
	else if (at_ac && !in_in)
	{
	    statement overlapstat;

	    pips_debug(7, "compiling a parallel loop\n");

	    if (Overlap_Analysis(stat, &overlapstat))
	    {
		debug(7, "hpf_compile_loop", "overlap analysis succeeded\n");

		*hoststatp = make_continue_statement(entity_empty_label());
		*nodestatp = overlapstat;
		statement_comments(*nodestatp) = statement_comments(stat);
	    }
	    else
	    {
		debug(7, "hpf_compile_loop", "overlap analysis is not ok...\n");

		hpf_compile_parallel_loop(stat, hoststatp, nodestatp);
	    }
	}
	else
	{
	    debug(7,"hpf_compile_loop",
		  "compiling a parallel loop sequential...\n");
	    hpf_compile_sequential_loop(stat, hoststatp, nodestatp);
	}
    }
    else
    {
	debug(7,"hpf_compile_loop","compiling a sequential loop\n");
    
	hpf_compile_sequential_loop(stat, hoststatp, nodestatp);
    }
}

/* what: compile a statement into a host and SPMD node code.
 * how: double code rewriting in a recursive traversal of stat.
 * input: statement stat.
 * output: statements *hoststatp and *nodestatp
 * side effects: ?
 * bugs or features:
 *  - special care is made here of I/O and remappings.
 */
void 
hpf_compiler(
    statement stat,
    statement *hoststatp,
    statement *nodestatp)
{
    list /* of hpfc_reduction */ lr = NIL;

    if (load_statement_only_io(stat)==1) /* necessary */
    {
	io_efficient_compile(stat,  hoststatp, nodestatp);
	return;
    }
    else if (bound_renamings_p(stat)) /* remapping */
    {
	remapping_compile(stat, hoststatp, nodestatp);
	return;
    }
    
    if (bound_hpf_reductions_p(stat)) /* HPF REDUCTION */
	lr = handle_hpf_reduction(stat);
    
    /* else usual stuff 
     */
    switch(instruction_tag(statement_instruction(stat)))
    {
    case is_instruction_block:
	hpf_compile_block(stat, hoststatp, nodestatp);
	break;
    case is_instruction_test:
	hpf_compile_test(stat, hoststatp, nodestatp);
	break;
    case is_instruction_loop:
	hpf_compile_loop(stat, hoststatp, nodestatp);
	break;
    case is_instruction_call:
	hpf_compile_call(stat, hoststatp, nodestatp);
	break;
    case is_instruction_unstructured:
	hpf_compile_unstructured(stat, hoststatp, nodestatp);
	break;
    case is_instruction_goto:
    default:
	pips_internal_error("unexpected instruction tag\n");
	break;
    }

    if (lr)
    {
	list /* of statement */ lh, ln;
	lh = gen_nconc(compile_hpf_reduction(lr, TRUE, TRUE),
         CONS(STATEMENT, *hoststatp,
	               compile_hpf_reduction(lr, FALSE, TRUE)));
	ln = gen_nconc(compile_hpf_reduction(lr, TRUE, FALSE),
	 CONS(STATEMENT, *nodestatp,
	               compile_hpf_reduction(lr, FALSE, FALSE)));

	*hoststatp = make_block_statement(lh);
	*nodestatp = make_block_statement(ln);
    }
}

/*   That is all for $RCSfile: compiler.c,v $
 */
