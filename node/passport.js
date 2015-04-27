'use strict';

exports = module.exports = function(app, passport) {
  var LocalStrategy = require('passport-local').Strategy,
      TwitterStrategy = require('passport-twitter').Strategy,
      GitHubStrategy = require('passport-github').Strategy,
      FacebookStrategy = require('passport-facebook').Strategy,
      GoogleStrategy = require('passport-google-oauth').OAuth2Strategy,
      TumblrStrategy = require('passport-tumblr').Strategy;

  passport.use(new LocalStrategy(
    function(username, password, done) {
      var conditions = { isActive: 'yes' };
      var database = 'debug_dspdb';
      var testQuery = 'SELECT * FROM '+database+'.accounts WHERE ';
      if (username.indexOf('@') === -1) {
        conditions.username = username;
        testQuery += 'login="'+username+'" AND';
      }
      else {
        conditions.email = username.toLowerCase();
        testQuery += 'email="'+conditions.email+'" AND';
      }
      testQuery +=' password=password("'+password+'")';
      app.sql.query(testQuery, function(err, rows, fields) {
        if (err){
          console.log('error');
          return done(err);
        }
        else{
          if(rows.length > 0){
            return done(null, rows[0]);
          }
          return done(null, false, { message: 'Unknown user' });
        }
      });
    }
  ));

  if (app.config.oauth.twitter.key) {
    passport.use(new TwitterStrategy({
        consumerKey: app.config.oauth.twitter.key,
        consumerSecret: app.config.oauth.twitter.secret
      },
      function(token, tokenSecret, profile, done) {
        done(null, false, {
          token: token,
          tokenSecret: tokenSecret,
          profile: profile
        });
      }
    ));
  }

  if (app.config.oauth.github.key) {
    passport.use(new GitHubStrategy({
        clientID: app.config.oauth.github.key,
        clientSecret: app.config.oauth.github.secret,
        customHeaders: { "User-Agent": app.config.projectName }
      },
      function(accessToken, refreshToken, profile, done) {
        done(null, false, {
          accessToken: accessToken,
          refreshToken: refreshToken,
          profile: profile
        });
      }
    ));
  }

  if (app.config.oauth.facebook.key) {
    passport.use(new FacebookStrategy({
        clientID: app.config.oauth.facebook.key,
        clientSecret: app.config.oauth.facebook.secret
      },
      function(accessToken, refreshToken, profile, done) {
        done(null, false, {
          accessToken: accessToken,
          refreshToken: refreshToken,
          profile: profile
        });
      }
    ));
  }

  if (app.config.oauth.google.key) {
    passport.use(new GoogleStrategy({
        clientID: app.config.oauth.google.key,
        clientSecret: app.config.oauth.google.secret
      },
      function(accessToken, refreshToken, profile, done) {
        done(null, false, {
          accessToken: accessToken,
          refreshToken: refreshToken,
          profile: profile
        });
      }
    ));
  }

  if (app.config.oauth.tumblr.key) {
    passport.use(new TumblrStrategy({
        consumerKey: app.config.oauth.tumblr.key,
        consumerSecret: app.config.oauth.tumblr.secret
      },
      function(token, tokenSecret, profile, done) {
        done(null, false, {
          token: token,
          tokenSecret: tokenSecret,
          profile: profile
        });
      }
    ));
  }

  passport.serializeUser(function(user, done) {
    done(null, user.id);
  });

  passport.deserializeUser(function(id, done) {
    var database = 'debug_dspdb';
    var testQuery = 'SELECT * FROM '+database+'.accounts WHERE id="'+id+'"';
    app.sql.query(testQuery, function(err, rows, fields) {
        if (err){
          console.log('error');
          return done(err, null);
        }
        else{
          return done(err, rows[0]);
        }
      });
  });
};
