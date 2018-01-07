$(document).ready(function () {
    $('#useful_cmd').combobox({
        showItemIcon: true,
        data: [
            { value: 'add', text: '关机', iconCls: 'icon-add' },
            { value: 'del', text: '重启', iconCls: 'icon-remove' },
        ],
        editable: false,
        panelHeight: 'auto',
    });

    $('#tt').tabs({
        height: 'auto'
    });
});
