'use strict';

var app = angular.module('acornWebApp', [
  'ngRoute',
  'ngResource',
  'toastr',
  'ngCookies'
])
.config(function($routeProvider, $locationProvider) {
  $routeProvider
    .when('/app/', {
      templateUrl: "app/views/home.html",
      controller: "HomeCtrl"
    })
    .when('/app/squirrels', {
      templateUrl: "app/views/squirrels.html",
      controller: "SquirrelCtrl"
    })
    .when('/app/dashboard', {
      templateUrl: "app/views/dashboard.html",
      controller: "DashboardCtrl"
    })
    .otherwise({
      templateUrl: "app/views/404.html"
    });

  $locationProvider.html5Mode(true);
});

app.filter('bytes', function() {
  return function(bytes, precision) {
      if (bytes === 0) { return '0 bytes' }
      if (isNaN(parseFloat(bytes)) || !isFinite(bytes)) return '-';
      if (typeof precision === 'undefined') precision = 1;

      var units = ['bytes', 'kB', 'MB', 'GB', 'TB', 'PB'],
          number = Math.floor(Math.log(bytes) / Math.log(1024)),
          val = (bytes / Math.pow(1024, Math.floor(number))).toFixed(precision);

      return  (val.match(/\.0*$/) ? val.substr(0, val.indexOf('.')) : val) +  ' ' + units[number];
  }
}).filter('secondsToDateTime', function($filter) {
    return function(seconds) {
        return new Date(0, 0, 0).setSeconds(seconds);
    };
});
