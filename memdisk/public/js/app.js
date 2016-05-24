'use strict';

var app = angular.module('acornWebApp', [
  'ngRoute',
  'ngResource'
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
    });

  $locationProvider.html5Mode(true);
});
