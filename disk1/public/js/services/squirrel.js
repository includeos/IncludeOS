'use strict';

angular.module('acornWebApp')
  .factory('Squirrel', ['$resource', function($resource) {
    return $resource('/api/squirrels');
  }]);
