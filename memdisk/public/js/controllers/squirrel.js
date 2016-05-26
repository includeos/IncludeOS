'use strict';

angular.module('acornWebApp')
  .controller('SquirrelCtrl', ['$scope', 'Squirrel', function($scope, Squirrel) {

    $scope.squirrel = new Squirrel();
    $scope.squirrels = [];

    $scope.squirrels = Squirrel.query(function() {});

    $scope.create = function() {
      $scope.squirrel.$save(
        function(squirrel, headers) {

        },
        function(error) {
          console.log("error");
        }
      );
    };
  }]);
