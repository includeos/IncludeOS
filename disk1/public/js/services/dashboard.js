'use strict';

angular.module('acornWebApp')
  .factory('Dashboard', ['$resource', function($resource) {
    return $resource('/api/dashboard', null, {
      query: {method: 'get', isArray: false }
    });
  }]);
