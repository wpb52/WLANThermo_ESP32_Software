/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

ipAddress = "";

// Wait for the deviceready event before using any of Cordova's device APIs.
// See https://cordova.apache.org/docs/en/latest/cordova/events/events.html#deviceready
document.addEventListener('deviceready', onDeviceReady, false);

function onDeviceReady() {
    // Cordova is now initialized. Have fun!
    var zeroconf = cordova.plugins.zeroconf;
    
    zeroconf.registerAddressFamily = 'ipv4';
    zeroconf.watchAddressFamily = 'ipv4';
    
    zeroconf.watch('_wlanthermo._tcp.', 'local.', function(result) {
    var action = result.action;
    var service = result.service;
    if (action == 'added') {
        console.log('service added', service);
    } else if (action == 'resolved') {
        console.log('service resolved', service);
        console.log('device name: ' + service.txtRecord.device);
        console.log('device ip: ' + service.ipv4Addresses[0]);
        ipAddress = service.ipv4Addresses[0];
        window.location.href='device.html?ip=' + ipAddress;
    } else {
        console.log('service removed', service);
    }
});
}
