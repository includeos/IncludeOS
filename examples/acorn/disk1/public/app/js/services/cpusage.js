'use strict';

angular.module('acornWebApp')
  .factory('CPUsage', function() {
    // CPU usage chart
    var time_data = ['x', new Date()];
    var idle_data = ['idle'];
    var active_data = ['active'];

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
            idle_data,
            active_data
          ],
          colors: {
            idle: '#3A8BF1',
            active: '#F87E0C'
          },
          types: {
            idle: 'area',
            active: 'area'
          },
          area : {
            zerobased: true
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
              text: 'percent',
              position: 'outer-middle'
            },
            tick: {
              format: function (d) {
                return d + " %";
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

      if(idle_data.length > 20) {
        // Remove second element in each array (first element is name)
        time_data.splice(1, 1);
        idle_data.splice(1, 1);
        active_data.splice(1, 1);
      }

      time_data.push(new Date());
      idle_data.push(usage.idle.toFixed(3));
      active_data.push(usage.active.toFixed(3));

      cpu_usage_chart.axis.labels({x: 'CPU usage over time'});
      cpu_usage_chart.load({
        columns: [
          time_data,
          idle_data,
          active_data
        ]
      });
    }

    return CPUsage;
  });
