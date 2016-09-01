'use strict';

angular.module('acornWebApp')
  .controller('DashboardCtrl', ['$scope', 'Dashboard', function($scope, Dashboard) {

    $scope.dashboard = new Dashboard();
    $scope.memmap = [];
    $scope.statman = [];
    $scope.stack_sampler = [];
    $scope.status = {};

    var data = Dashboard.query(function() {
      $scope.memmap = data.memmap;
      $scope.statman = data.statman;
      $scope.stack_sampler = data.stack_sampler;
      $scope.status = data.status;
    });
  }]);
