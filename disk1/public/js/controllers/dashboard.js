'use strict';

angular.module('acornWebApp')
  .controller('DashboardCtrl',
    ['$scope', 'Dashboard', '$timeout', '$http', '$interval',
    function($scope, Dashboard, $timeout, $http, $interval) {

    // TODO: How access bytes-function in app.js here?
    function bytes(bytes, precision) {
      if (bytes === 0) { return '0 bytes' }
      if (isNaN(parseFloat(bytes)) || !isFinite(bytes)) return '-';
      if (typeof precision === 'undefined') precision = 1;

      var units = ['bytes', 'kB', 'MB', 'GB', 'TB', 'PB'],
          number = Math.floor(Math.log(bytes) / Math.log(1024)),
          val = (bytes / Math.pow(1024, Math.floor(number))).toFixed(precision);

      return  (val.match(/\.0*$/) ? val.substr(0, val.indexOf('.')) : val) +  ' ' + units[number];
    }

    // CPU usage chart
    var date = new Date();
    var total_data = ['total'];
    var active_data = ['active'];
    var time_data = ['x', date];

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

        // Only room for 10 elements in chart
        /*var empty = ['Unused', memmap[0].addr_start];
        memmap_columns.push(empty);
        groups.push('Unused');*/

        angular.forEach(memmap, function(element, index) {
          var element_data = [(index + 1) + ': ' + element.name, (element.addr_end - element.addr_start) + 0x1];
          memmap_columns.push(element_data);

          // If group name not identical with element name
          // - one bar per element instead of one bar for all
          groups.push((index + 1) + ': ' + element.name);
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
            // colors: {name: '#ff0000', name2: '#ff0000'},
            type: 'bar',
            groups: [
              groups
            ],
            order: null
          },
          axis: {
            x: {
              show: false,
              label: {
                text: "Elements"
              }
            },
            y: {
              tick: {
                format: function (y) { return bytes(y); }
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
                  bytes(map_element.in_use) + ' (' + ((map_element.in_use / value)*100).toFixed(0) + '%)';
              }
            }
          }
        });
      }).
      error(function (data, status) {

      });

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

          total_data.push(total);
          active_data.push(active);
          time_data.push(d);

          cpu_usage_chart.axis.labels({x: 'CPU data updated at an interval of ' + interval + ' seconds'});
          cpu_usage_chart.load({
            columns: [
              time_data,
              total_data,
              active_data
            ]
          });
        }).
        error(function (data, status) {

        });
    };

    var cpu_update = $interval($scope.update_cpu_chart, 3000);

    /* Polling CPU usage data and updating chart
    var polling_cpu;

    (function poll_cpu() {
      var data = Dashboard.query(function() {
        var total = data.cpu_usage.total;
        var halt = data.cpu_usage.halt;
        var active = total - halt;
        var d = new Date();

        total /= 1000000;  // 1 million
        active /= 1000000;

        total_data.push(total);
        active_data.push(active);
        time_data.push(d);

        cpu_usage_chart.load({
          columns: [
            time_data,
            total_data,
            active_data
          ]
        });

        polling_cpu = $timeout(poll_cpu, 2500);
      });
    })();*/

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
