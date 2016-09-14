'use strict';

angular.module('acornWebApp')
  .controller('HomeCtrl', ['$cookies', function($cookies) {

    var chosen_language = $cookies.get('lang');

    if(chosen_language == "en-US") {
      console.log("English language chosen!");
    } else if(chosen_language == "nb-NO") {
      console.log("Norwegian language chosen!");
    } else {
      console.log("No language chosen!");
    }
  }]);
