'use strict';

var app = angular.module('acornWebApp', [
  'ngRoute',
  'ngResource',
  'toastr',
  'ngCookies'
])
.config(function($routeProvider, $locationProvider) {
  $routeProvider
    .when('/', {
      templateUrl: "views/home.html",
      controller: "HomeCtrl"
    })
    .when('/squirrels', {
      templateUrl: "views/squirrels.html",
      controller: "SquirrelCtrl"
    })
    .otherwise({
      templateUrl: "views/404.html"
    });

  $locationProvider.html5Mode(true);
});
