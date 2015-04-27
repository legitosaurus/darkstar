var ws = {};

ws.init = function(app){
  console.log('ws init');
  app.io.on('connection', function(socket){
    console.log('a user connected');
  });
}


module.exports = ws;