var login = {};
login.init = function(app){
  var database = 'debug_dspdb';
  var testUser = 'foodz';
  var testPassword = '12261226'; //12261226
  var testQuery = 'SELECT * FROM '+database+'.accounts WHERE login="'+testUser+'" AND password=password("'+testPassword+'")';
  app.sql.query(testQuery, function(err, rows, fields) {
  if (err){
    console.log('error');
    console.dir(err);
  }
  else{
    console.log('no error!');
    console.dir(rows);
  }
});

}


module.exports = login;