var express = require('express');
var router = express.Router();

var pressed = 0;

/* GET home page. */
router.get('/', function(req, res) {
    res.render('index', { title: 'PewPew Controller' });
});

router.post('/spacebar', function(req, res) {
    pressed = (pressed + 1) % 2;
    console.log("Pressed: " + pressed);
    res.end();
});

router.get('/spacebar', function(req, res) {
    res.status(200).send(pressed.toString());
})

module.exports = router;
