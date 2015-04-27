var xi = {};
xi.ws = require('./webSocket');
xi.connector = require('./xiConnector');
xi.login = require('./xiLogin');


xi.init = function(app){
  xi.connector.init(app);
  xi.login.init(app);
  xi.ws.init(app);
}

exports = module.exports = xi;