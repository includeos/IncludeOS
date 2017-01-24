'use strict';

angular.module('acornWebApp')
  .controller('DashboardCtrl',
    ['$scope', 'Dashboard', '$timeout', '$http', '$interval', 'bytesFilter', 'CPUsage',
    function($scope, Dashboard, $timeout, $http, $interval, bytesFilter, CPUsage) {

    var color_palette = ['#F8D20B', '#3A8BF1', '#0D1230', '#3452CB', '#B5D63B', '#A061F2', '#F87E0C', '#EE4053'];

    // Memory map chart
    $http.get("/api/dashboard/memmap").
      success(function (memmap) {
        var memmap_columns = [];
        var groups = [];
        var groups_with_colors = {};
        var color_index = 0;
        var hidden_groups = [];

        angular.forEach(memmap, function(element, index) {
          var element_name = (index + 1) + ': ' + element.name;
          var element_data = [element_name, (element.addr_end - element.addr_start) + 0x1];

          memmap_columns.push(element_data);
          groups.push(element_name);

          if(index < color_palette.length) {
            groups_with_colors[element_name] = color_palette[index];
          }
          else {
            if(color_index >= color_palette.length)
              color_index = 0;

            groups_with_colors[element_name] = color_palette[color_index++];
          }

          if(element.name == "N/A")
            hidden_groups.push(element_name);
        });

        // Memory map chart
        var memory_map_chart = c3.generate({
          bindto: '#memory_map_chart',
          padding: {
            left: 20,
            right: 20
          },
          data: {
            columns: memmap_columns,
            colors: groups_with_colors,
            type: 'bar',
            groups: [
              groups
            ],
            hide: hidden_groups,
            order: null
          },
          axis: {
            x: {
              show: false
            },
            y: {
              tick: {
                format: function (y) { return bytesFilter(y); }
              },
              padding: {
                top: 0
              }
            },
            rotated: true
          },
          tooltip: {
            format: {
              title: function(d) { return "Memory"; },
              name: function(name, ratio, id) {
                var parts = id.split(/:/);
                var index = parts[0] - 1;
                return parts[1] + ': ' + memmap[index].description;
              },
              value: function (value, ratio, id) {
                var parts = id.split(/:/);
                var index = parts[0] - 1;
                var map_element = memmap[index];
                return '0x' + map_element.addr_start.toString(16) +
                  ' - 0x' + map_element.addr_end.toString(16) + ' In use: ' +
                  bytesFilter(map_element.in_use) + ' (' + ((map_element.in_use / value)*100).toFixed(0) + '%)';
              }
            }
          }
        });
      }).
      error(function (data, status) {});

    var cpusage = new CPUsage('#cpu_usage_chart');

    // Polling dashboard data
    $scope.dashboard = new Dashboard();
    $scope.memmap = [];
    $scope.statman = [];
    $scope.stack_sampler = {};
    $scope.cpu_usage = {};
    $scope.status = {};
    $scope.tcp = {};
    $scope.logger = [];

    // Update CPU chart everytime $scope.cpu_usage changes
    $scope.$watch('cpu_usage', function() {
      cpusage.update($scope.cpu_usage);
    });

    $scope.statTree = [];

    var createTree = function(statman) {
      // tree is an object (root) containing an array of nodes
      var tree = { nodes: [] };

      for (var i = 0; i < statman.length; i++)
        fillTree(tree, statman[i].name, statman[i].value);

      $scope.statTree = tree.nodes;
    }

    var polling;

    (function poll() {
      var data = Dashboard.query(function() {
        //$scope.memmap = data.memmap;
        $scope.statman = data.statman;
        $scope.stack_sampler = data.stack_sampler;
        $scope.cpu_usage = data.cpu_usage;
        $scope.status = data.status;
        $scope.tcp = data.tcp;
        $scope.logger = data.logger;

        createTree($scope.statman);

        polling = $timeout(poll, 1000);
      });
    })();

    $scope.uptime = 0;

    var uptime = $interval(function() {
      $scope.uptime = countdown( new Date($scope.status.boot_time), null ).toString();
    }, 1000);

    $scope.$on('$destroy', function(){
      $timeout.cancel(polling);
      $interval.cancel(uptime);
    })
  }]);
