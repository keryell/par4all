$(function(){

    // A+ / A- buttons
    $('#aplus').click(function(ev) {
	ev.preventDefault();
	resize(1)
    });
    $('#aminus').click(function(ev) {
	ev.preventDefault();
	resize(0)
    });

    // Classic examples
    $('#classic-examples-dialog').modal();
    $('#classic-examples-dialog a').click(function(ev) {
	ev.preventDefault();
	load_example(ev.target.id);
    });

    // File upload
    $('#upload_input').change(function() {
	$('#upload_form').submit();
	$('#upload_target').load(after_upload);
    });

    // Source code input field
    $('#sourcecode-1').linedtextarea()
	.attr('spellcheck', false);

    // "Choose function" dialog
    $('#choose-function-dialog').modal();

    // "RESULTS" tab
    $('#result_tab a').click(function(ev) {
	ev.preventDefault();
	if (performed == false)
	    performed = perform_operation(1, 'result');
    });
    // "GRAPH" tab
    $('#graph_tab a').click(function(ev) {
	ev.preventDefault();
	if (created_graph == false)
	    created_graph = create_graph(1, 'graph');
    });

    // Run button
    $("#run-button").click(function(ev) {
	ev.preventDefault();
	activate_tab('result_tab_link');
	if (performed == false)
	    performed = perform_operation(1, 'result');
    });

    // Print buttons
    $("#print-button").click(function(ev) {
        var content    = $('#resultcode'),
            printFrame = $('#iframetoprint').contentWindow;
        printFrame.document.ope();
        printFrame.document.write(content.innerHTML);
        printFrame.document.close();
        printFrame.focus();
        printFrame.print();
    });
    deactivate_buttons();    

});
