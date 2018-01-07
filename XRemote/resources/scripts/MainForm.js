$(document).ready(function () {

    $(window).bind('resize', function () {
        var height = $(document.body).height();
        var left_panel = $('#left_panel');
        var margin_tb = parseInt(left_panel.css('margin-top')) + parseInt(left_panel.css('margin-bottom'));
        height -= margin_tb;
        left_panel.css('height', height + 'px');
        left_panel.css('width', '20%');

        var width = $(document.body).width();
        var right_panel = $('#right_panel');
        right_panel.css('height', height + 'px');
        var outerWidth_lp = left_panel.outerWidth(true);
        right_panel.css('left', outerWidth_lp + 5 + 'px');
    });

    $(window).resize();

    $('#tt').tabs({ justified: true });
});


