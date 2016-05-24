'use strict';

angular.module('acornWebApp')
  .controller('SquirrelCtrl', ['$scope', 'Squirrel', function($scope, Squirrel) {

    $scope.squirrels = [];

    $scope.squirrels = Squirrel.query(function() {});
  }]);
