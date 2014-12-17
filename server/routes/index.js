var express = require('express');
var router = express.Router();

var spaceBarPressed = 0;
var headPosition = {x: 0, y: 0, z: 0};
var timestamp = 0;

/* Helper function. */
var pad5 = function(number) {
    return String("00000" + number).slice(-5);
}

var pad3 = function(number) {
    return String("000" + number).slice(-3);
}

/* GET home page. */
router.get('/', function(req, res) {
    res.render('index', { title: 'PewPew Controller' });
});

router.post('/spacebar', function(req, res) {
    spaceBarPressed = (spaceBarPressed + 1) % 2;
    console.log("Pressed        : " + spaceBarPressed);
    res.end();
});

router.get('/spacebar', function(req, res) {
    res.status(200).send(spaceBarPressed.toString());
});

router.post('/position', function(req, res) {
    if (timestamp < req.query.t) {
        timestamp = req.query.t;
        headPosition.x = req.query.x;
        headPosition.y = req.query.y;
        headPosition.z = req.query.z;
        console.log("Player Position: " + headPosition);
        console.log("Timestamp      : " + timestamp);
    }
    res.end();
});

router.get('/timestamp', function(req, res) {
    res.status(200).send(timestamp);
    res.end();
});

router.get('/position', function(req, res) {
    res.status(200).send(headPosition.x + " " + headPosition.y + " " + headPosition.z);
});

module.exports = router;
