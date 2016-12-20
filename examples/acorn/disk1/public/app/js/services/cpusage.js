'use strict';

angular.module('acornWebApp')
  .factory('CPUsage', function() {
    // CPU usage chart
    var date = new Date();
    var total_data = ['total'];
    var active_data = ['active'];
    var time_data = ['x', date];

    var cpu_usage_chart = {};

    var setup = function(html_id) {
      cpu_usage_chart = c3.generate({
        bindto: html_id,
        padding: {
          right: 40,
          top: 40
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
    };

    function CPUsage(html_id) {
      setup(html_id);
    };

    CPUsage.prototype.update = function(usage) {
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
    }

    return CPUsage;
  });
