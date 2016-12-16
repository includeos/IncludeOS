'use strict';

angular.module('acornWebApp')
  .controller('SquirrelCtrl', ['$scope', 'Squirrel', 'toastr', function($scope, Squirrel, toastr) {

    $scope.squirrel = new Squirrel();
    $scope.squirrels = [];

    $scope.squirrels = Squirrel.query(function() {});

    $scope.create = function() {
      $scope.squirrel.$save({},
        function(squirrel, headers) {
          $scope.squirrels.push(squirrel);
          $scope.squirrel = new Squirrel();
          toastr.success('Squirrel captured!', 'Success');
        },
        function(error) {
          //console.log(error);
          toastr.error(error.data.message, error.data.type);
        }
      );
    };
  }]);
