'use strict';

angular.module('acornWebApp')
  .controller('DashboardCtrl',
    ['$scope', 'Dashboard', '$timeout', '$http', '$interval', 'bytesFilter',
    function($scope, Dashboard, $timeout, $http, $interval, bytesFilter) {

    // CPU usage chart
    var date = new Date();
    var total_data = ['total'];
    var active_data = ['active'];
    var time_data = ['x', date];
    var color_palette = ['#A061F2', '#3A8BF1', '#3452CB', '#0D1230', '#EE4053', '#F87E0C', '#F8D20B', '#B5D63B'];

    var cpu_usage_chart = c3.generate({
      bindto: '#cpu_usage_chart',
      padding: {
        right: 20
      },
      data: {
        x: 'x',
        columns: [
          time_data,
          total_data,
          active_data
        ],
        colors: {
          total: '#3A8BF1',
          active: '#F87E0C'
        },
        types: {
          total: 'area-spline',
          active: 'area-spline'
          // 'line', 'spline', 'step', 'area', 'area-step' are also available to stack
        }
      },
      axis: {
        x: {
          type: 'timeseries',
          tick: {
            format: '%H:%M:%S'
          },
          label: {
            position: 'outer-left',
            padding: {
              top: 100,
              left: 100
            }
          }
        },
        y: {
          label: {
            text: 'cycles',
            position: 'outer-middle'
          },
          tick: {
            format: function (d) {
              return d + " mill.";
            }
          }
        }
      }
    });

    // Memory map chart
    $http.get("/api/dashboard/memmap").
      success(function (memmap) {
        var memmap_columns = [];
        var groups = [];
        var group_with_colors = {};
        var color_index = 0;

        angular.forEach(memmap, function(element, index) {
          var element_name = (index + 1) + ': ' + element.name;
          var element_data = [element_name, (element.addr_end - element.addr_start) + 0x1];

          memmap_columns.push(element_data);
          groups.push(element_name);

          if(index < color_palette.length) {
            group_with_colors[element_name] = color_palette[index];
          }
          else {
            if(color_index >= color_palette.length)
              color_index = 0;

            group_with_colors[element_name] = color_palette[color_index++];
          }
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
            colors: group_with_colors,
            type: 'bar',
            groups: [
              groups
            ],
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
      }).error(function (data, status) {});

    // Update CPU usage chart at interval
    $scope.update_cpu_chart = function() {

      $http.get("/api/dashboard/cpu_usage").
        success(function (usage) {

          // Showing interval in number of seconds
          var interval = usage.interval / 1000000;

          if(total_data.length > 20) {
            // Remove second element in each array (first element is name)
            total_data.splice(1, 1);
            active_data.splice(1, 1);
            time_data.splice(1, 1);
          }

          var total = usage.total;
          var halt = usage.halt;
          var active = total - halt;

          // Showing cycles in millions
          total /= 1000000;
          active /= 1000000;

          var d = new Date();

          total_data.push(total.toFixed(3));
          active_data.push(active.toFixed(3));
          time_data.push(d);

          cpu_usage_chart.axis.labels({x: 'CPU data updated at an interval of ' + interval + ((interval > 1) ? ' seconds' : ' second')});
          cpu_usage_chart.load({
            columns: [
              time_data,
              total_data,
              active_data
            ]
          });
        }).
        error(function (data, status) {});
    };

    var cpu_update = $interval($scope.update_cpu_chart, 1500);

    // Polling dashboard data
    $scope.dashboard = new Dashboard();
    $scope.memmap = [];
    $scope.statman = [];
    $scope.stack_sampler = [];
    $scope.status = {};
    $scope.tcp = {};
    $scope.logger = [];

    var polling;

    (function poll() {
      var data = Dashboard.query(function() {
        //$scope.memmap = data.memmap;
        $scope.statman = data.statman;
        $scope.stack_sampler = data.stack_sampler;
        $scope.status = data.status;
        $scope.tcp = data.tcp;
        $scope.logger = data.logger;
        //$scope.logger = data.logger.reverse();

        polling = $timeout(poll, 1000);
      });
    })();

    $scope.$on('$destroy', function(){
      //$timeout.cancel(polling_cpu);
      $interval.cancel(cpu_update);
      $timeout.cancel(polling);
    })
  }]);
