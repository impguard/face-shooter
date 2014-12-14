$(function() {
    var url = "http://localhost:8000/"
    var pressed = false;

    $('body').keydown(function(e) {
        var keycode = e.which || e.keyCode;
        if (keycode == 32 && !pressed) {
            // user has pressed space
            pressed = true;
            $.ajax({
                url: url + "spacebar",
                type: "POST",
            });
            console.log("pushed");
        }
    });

    $('body').keyup(function (e) {
        var keycode = e.which || e.keyCode;
        if (keycode == 32 && pressed) {
            // user has released space
            pressed = false;
            $.ajax({
                url: url + "spacebar",
                type: "POST",
            });
            console.log("released");
        }
    });

    $('body').keyup(function (e) {
        var keycode = e.which || e.keyCode;
        if (keycode == 13) {
            $.ajax({
                url: url + "spacebar",
                type: "GET",
                success: function(data, status, jqXHR) {
                    console.log("Spacebar  : " + data);
                }
            });
            $.ajax({
                url: url + "position",
                type: "GET",
                success: function(data, status, jqXHR) {
                    console.log("Position  : " + data);
                }
            });
        }
    });
});