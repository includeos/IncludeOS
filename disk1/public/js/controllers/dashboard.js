'use strict';

angular.module('acornWebApp')
  .controller('DashboardCtrl',
    ['$scope', 'Dashboard', '$timeout',
    function($scope, Dashboard, $timeout) {

    $scope.dashboard = new Dashboard();
    $scope.memmap = [];
    $scope.statman = [];
    $scope.stack_sampler = [];
    $scope.status = {};
    $scope.tcp = {};

    var polling;

    (function poll() {
      var data = Dashboard.query(function() {
        $scope.memmap = data.memmap;
        $scope.statman = data.statman;
        $scope.stack_sampler = data.stack_sampler;
        $scope.status = data.status;
        $scope.tcp = data.tcp;

        polling = $timeout(poll, 1000);
      });
    })();

    $scope.$on('$destroy', function(){
      $timeout.cancel(polling);
    })
  }]);
