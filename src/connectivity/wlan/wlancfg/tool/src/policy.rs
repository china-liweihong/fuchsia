// Copyright 2020 The Fuchsia Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

use {
    crate::opts::*,
    anyhow::{format_err, Error},
    fidl::endpoints::{create_endpoints, create_proxy},
    fidl_fuchsia_wlan_common as fidl_wlan_common, fidl_fuchsia_wlan_policy as wlan_policy,
    fuchsia_component::client::connect_to_service,
    futures::TryStreamExt,
};

// String formatting, printing, and general boilerplate helpers.

/// Communicates with the client policy provider to get the components required to get a client
/// controller.
pub fn get_client_controller(
) -> Result<(wlan_policy::ClientControllerProxy, wlan_policy::ClientStateUpdatesRequestStream), Error>
{
    let policy_provider = connect_to_service::<wlan_policy::ClientProviderMarker>()?;
    let (client_controller, server_end) =
        create_proxy::<wlan_policy::ClientControllerMarker>().unwrap();
    let (update_client_end, update_server_end) =
        create_endpoints::<wlan_policy::ClientStateUpdatesMarker>().unwrap();
    let () = policy_provider.get_controller(server_end, update_client_end)?;
    let update_stream = update_server_end.into_stream()?;

    Ok((client_controller, update_stream))
}

/// Communicates with the client listener service to get a stream of client state updates.
pub fn get_listener_stream() -> Result<wlan_policy::ClientStateUpdatesRequestStream, Error> {
    let listener = connect_to_service::<wlan_policy::ClientListenerMarker>()?;
    let (client_end, server_end) =
        create_endpoints::<wlan_policy::ClientStateUpdatesMarker>().unwrap();
    listener.get_listener(client_end)?;
    let server_stream = server_end.into_stream()?;
    Ok(server_stream)
}

/// Returns the SSID and security type of a network identifier as strings.
fn extract_network_id(
    network_id: Option<fidl_fuchsia_wlan_policy::NetworkIdentifier>,
) -> Result<(String, String), Error> {
    match network_id {
        Some(id) => {
            let ssid = std::string::String::from_utf8(id.ssid).unwrap();
            let security_type = match id.type_ {
                fidl_fuchsia_wlan_policy::SecurityType::None => "",
                fidl_fuchsia_wlan_policy::SecurityType::Wep => "wep",
                fidl_fuchsia_wlan_policy::SecurityType::Wpa => "wpa",
                fidl_fuchsia_wlan_policy::SecurityType::Wpa2 => "wpa2",
                fidl_fuchsia_wlan_policy::SecurityType::Wpa3 => "wpa3",
            };
            return Ok((ssid, security_type.to_string()));
        }
        None => return Ok(("".to_string(), "".to_string())),
    };
}

/// Returns a BSS's BSSID as a string.
fn extract_bss_info(bss_info: wlan_policy::Bss) -> (String, i8, u32) {
    let bssid = match bss_info.bssid {
        Some(bssid) => hex::encode(bssid),
        None => "".to_string(),
    };
    let rssi = match bss_info.rssi {
        Some(rssi) => rssi,
        None => 0,
    };
    let frequency = match bss_info.frequency {
        Some(frequency) => frequency,
        None => 0,
    };

    (bssid, rssi, frequency)
}

/// Returns WLAN compatilibity information as a string.
fn extract_compatibility(compatibility: Option<wlan_policy::Compatibility>) -> String {
    if compatibility.is_some() {
        match compatibility.unwrap() {
            wlan_policy::Compatibility::Supported => return "supported".to_string(),
            wlan_policy::Compatibility::DisallowedInsecure => {
                return "disallowed, insecure".to_string();
            }
            wlan_policy::Compatibility::DisallowedNotSupported => {
                return "disallowed, not supported".to_string();
            }
        }
    }

    return "compatilibity unknown".to_string();
}

/// Translates command line input into a network configuration.
fn construct_network_config(config: PolicyNetworkConfig) -> wlan_policy::NetworkConfig {
    let security_type = wlan_policy::SecurityType::from(config.security_type);
    let credential = match config.credential_type {
        CredentialTypeArg::r#None => wlan_policy::Credential::None(wlan_policy::Empty),
        CredentialTypeArg::Psk => {
            wlan_policy::Credential::Psk(config.credential.unwrap().as_bytes().to_vec())
        }
        CredentialTypeArg::Password => {
            wlan_policy::Credential::Password(config.credential.unwrap().as_bytes().to_vec())
        }
    };

    let network_id = wlan_policy::NetworkIdentifier {
        ssid: config.ssid.as_bytes().to_vec(),
        type_: security_type,
    };
    wlan_policy::NetworkConfig { id: Some(network_id), credential: Some(credential) }
}

/// Iterates through a vector of network configurations and prints their contents.
pub fn print_saved_networks(saved_networks: Vec<wlan_policy::NetworkConfig>) -> Result<(), Error> {
    for config in saved_networks {
        let (ssid, security_type) = extract_network_id(config.id)?;
        let password = match config.credential {
            Some(credential) => match credential {
                wlan_policy::Credential::None(wlan_policy::Empty) => String::from(""),
                wlan_policy::Credential::Password(bytes) => {
                    let password = std::string::String::from_utf8(bytes);
                    password.unwrap()
                }
                wlan_policy::Credential::Psk(bytes) => {
                    let password = std::string::String::from_utf8(bytes);
                    password.unwrap()
                }
                _ => return Err(format_err!("unknown credential variant detected")),
            },
            None => String::from(""),
        };
        println!("{:32} | {:4} | {}", ssid, security_type, password);
    }
    Ok(())
}

/// Iterates through a vector of scan results and prints each one.
pub fn print_scan_results(scan_results: Vec<wlan_policy::ScanResult>) -> Result<(), Error> {
    for network in scan_results {
        let (ssid, security_type) = extract_network_id(network.id)?;
        let compatibility = extract_compatibility(network.compatibility);
        println!("{:32} | {:3} ({})", ssid, security_type, compatibility);

        if network.entries.is_some() {
            for entry in network.entries.unwrap() {
                let (bssid, rssi, frequency) = extract_bss_info(entry);
                println!("\t0x{:12} | {:3} | {:5}", bssid, rssi, frequency);
            }
        }
    }
    Ok(())
}

/// Parses the return value from policy layer FIDL operations and prints a human-readable
/// representation of the result.
fn handle_request_status(status: fidl_wlan_common::RequestStatus) -> Result<(), Error> {
    match status {
        fidl_wlan_common::RequestStatus::Acknowledged => Ok(()),
        fidl_wlan_common::RequestStatus::RejectedNotSupported => {
            Err(format_err!("request failed: not supported"))
        }
        fidl_wlan_common::RequestStatus::RejectedIncompatibleMode => {
            Err(format_err!("request failed: incompatible mode"))
        }
        fidl_wlan_common::RequestStatus::RejectedAlreadyInUse => {
            Err(format_err!("request failed: already in use"))
        }
        fidl_wlan_common::RequestStatus::RejectedDuplicateRequest => {
            Err(format_err!("request failed: duplicate request."))
        }
    }
}

// Policy client helper functions

/// Issues a connect call to the client policy layer and waits for the connection process to
/// complete.
pub async fn handle_connect(
    client_controller: wlan_policy::ClientControllerProxy,
    mut server_stream: wlan_policy::ClientStateUpdatesRequestStream,
    network_id: PolicyNetworkId,
) -> Result<(), Error> {
    let security_type = wlan_policy::SecurityType::from(network_id.security_type);
    let mut network_id = wlan_policy::NetworkIdentifier {
        ssid: network_id.ssid.as_bytes().to_vec(),
        type_: security_type,
    };

    let result = client_controller.connect(&mut network_id).await?;
    handle_request_status(result)?;

    while let Some(update_request) = server_stream.try_next().await? {
        let update = update_request.into_on_client_state_update();
        let (update, responder) = match update {
            Some((update, responder)) => (update, responder),
            None => return Err(format_err!("Client provider produced invalid update.")),
        };
        let _ = responder.send();

        match update.state {
            Some(state) => {
                if state == wlan_policy::WlanClientState::ConnectionsDisabled {
                    return Err(format_err!("Connections disabled while trying to conncet."));
                }
            }
            None => continue,
        }

        let networks = match update.networks {
            Some(networks) => networks,
            None => continue,
        };

        for net_state in networks {
            if net_state.id.is_none() || net_state.state.is_none() {
                continue;
            }
            if net_state.id.unwrap().ssid == network_id.ssid {
                match net_state.state.unwrap() {
                    wlan_policy::ConnectionState::Failed => {
                        return Err(format_err!(
                            "Failed to connect with reason {:?}",
                            net_state.status
                        ));
                    }
                    wlan_policy::ConnectionState::Disconnected => {
                        return Err(format_err!("Disconnect with reason {:?}", net_state.status));
                    }
                    wlan_policy::ConnectionState::Connecting => continue,
                    wlan_policy::ConnectionState::Connected => return Ok(()),
                }
            }
        }
    }
    Err(format_err!("Status stream terminated before connection could be verified."))
}

/// Issues a call to the client policy layer to get saved networks and prints all saved network
/// configurations.
pub async fn handle_get_saved_networks(
    client_controller: wlan_policy::ClientControllerProxy,
) -> Result<Vec<wlan_policy::NetworkConfig>, Error> {
    let (client_proxy, server_end) =
        create_proxy::<wlan_policy::NetworkConfigIteratorMarker>().unwrap();
    match client_controller.get_saved_networks(server_end) {
        Ok(_) => (),
        Err(e) => return Err(format_err!("failed to get saved networks with {:?}", e)),
    }

    let mut saved_networks = Vec::new();

    loop {
        match client_proxy.get_next().await {
            Ok(mut new_configs) => {
                if new_configs.is_empty() {
                    break;
                }
                saved_networks.append(&mut new_configs);
            }
            Err(_) => return Err(format_err!("failed while retrieving saved networks")),
        }
    }
    Ok(saved_networks)
}

/// Listens for client state updates and prints each update that is received.
pub async fn handle_listen(
    mut server_stream: wlan_policy::ClientStateUpdatesRequestStream,
) -> Result<(), Error> {
    while let Some(update_request) = server_stream.try_next().await? {
        let update = update_request.into_on_client_state_update();
        let (update, responder) = match update {
            Some((update, responder)) => (update, responder),
            None => return Err(format_err!("Client provider produced invalid update.")),
        };
        let _ = responder.send();

        match update.state {
            Some(state) => match state {
                wlan_policy::WlanClientState::ConnectionsEnabled => {
                    println!("Client connections are enabled");
                }
                wlan_policy::WlanClientState::ConnectionsDisabled => {
                    println!("Client connections are disabled");
                }
            },
            None => {}
        }

        let networks = match update.networks {
            Some(networks) => networks,
            None => continue,
        };

        for net_state in networks {
            if net_state.id.is_none() || net_state.state.is_none() {
                continue;
            }
            let id = net_state.id.unwrap();
            let ssid = std::str::from_utf8(&id.ssid).unwrap();
            match net_state.state.unwrap() {
                wlan_policy::ConnectionState::Failed => {
                    println!("{:32}: connection failed", ssid);
                }
                wlan_policy::ConnectionState::Disconnected => {
                    println!("{:32}: connection failed", ssid);
                }
                wlan_policy::ConnectionState::Connecting => {
                    println!("{:32}: connecting", ssid);
                }
                wlan_policy::ConnectionState::Connected => println!("{:32}: connected", ssid),
            }
        }
    }
    Ok(())
}

/// Communicates with the client policy layer to remove a network.
pub async fn handle_remove_network(
    client_controller: wlan_policy::ClientControllerProxy,
    config: PolicyNetworkConfig,
) -> Result<(), Error> {
    let network_config = construct_network_config(config);
    match client_controller.remove_network(network_config).await? {
        Ok(()) => Ok(()),
        Err(e) => Err(format_err!("failed to remove network with {:?}", e)),
    }
}

/// Communicates with the client policy layer to save a network configuration.
pub async fn handle_save_network(
    client_controller: wlan_policy::ClientControllerProxy,
    config: PolicyNetworkConfig,
) -> Result<(), Error> {
    let network_config = construct_network_config(config);
    match client_controller.save_network(network_config).await? {
        Ok(()) => Ok(()),
        Err(e) => Err(format_err!("failed to save network with {:?}", e)),
    }
}

/// Issues a scan request to the client policy layer.
pub async fn handle_scan(
    client_controller: wlan_policy::ClientControllerProxy,
) -> Result<Vec<wlan_policy::ScanResult>, Error> {
    let (client_proxy, server_end) =
        create_proxy::<wlan_policy::ScanResultIteratorMarker>().unwrap();
    client_controller.scan_for_networks(server_end)?;

    let mut scanned_networks = Vec::<wlan_policy::ScanResult>::new();
    loop {
        match client_proxy.get_next().await? {
            Ok(mut new_networks) => {
                if new_networks.is_empty() {
                    break;
                }
                scanned_networks.append(&mut new_networks);
            }
            Err(e) => return Err(format_err!("Scan failure error: {:?}", e)),
        }
    }

    Ok(scanned_networks)
}

/// Requests that the policy layer start client connections.
pub async fn handle_start_client_connections(
    client_controller: wlan_policy::ClientControllerProxy,
) -> Result<(), Error> {
    let status = client_controller.start_client_connections().await?;
    return handle_request_status(status);
}

/// Asks the client policy layer to stop client connections.
pub async fn handle_stop_client_connections(
    client_controller: wlan_policy::ClientControllerProxy,
) -> Result<(), Error> {
    let status = client_controller.stop_client_connections().await?;
    return handle_request_status(status);
}

#[cfg(test)]
mod tests {
    use {
        super::*,
        crate::opts,
        fidl::endpoints,
        fidl_fuchsia_wlan_common as fidl_wlan_common, fidl_fuchsia_wlan_policy as wlan_policy,
        fuchsia_async::Executor,
        futures::{stream::StreamExt, task::Poll},
        pin_utils::pin_mut,
        wlan_common::assert_variant,
    };

    static TEST_SSID: &str = "test_ssid";

    struct ClientTestValues {
        client_proxy: wlan_policy::ClientControllerProxy,
        client_stream: wlan_policy::ClientControllerRequestStream,
        update_proxy: wlan_policy::ClientStateUpdatesProxy,
        update_stream: wlan_policy::ClientStateUpdatesRequestStream,
    }

    fn test_setup() -> ClientTestValues {
        let (client_proxy, client_stream) =
            endpoints::create_proxy_and_stream::<wlan_policy::ClientControllerMarker>()
                .expect("failed to create ClientController proxy");

        let (listener_proxy, listener_stream) =
            endpoints::create_proxy_and_stream::<wlan_policy::ClientStateUpdatesMarker>()
                .expect("failed to create ClientController proxy");

        ClientTestValues {
            client_proxy: client_proxy,
            client_stream: client_stream,
            update_proxy: listener_proxy,
            update_stream: listener_stream,
        }
    }

    /// Allows callers to respond to StartClientConnections, StopClientConnections, and Connect
    /// calls with the RequestStatus response of their choice.
    fn send_request_status(
        exec: &mut Executor,
        server: &mut wlan_policy::ClientControllerRequestStream,
        response: fidl_wlan_common::RequestStatus,
    ) {
        let poll = exec.run_until_stalled(&mut server.next());
        let request = match poll {
            Poll::Ready(poll_ready) => poll_ready
                .expect("poll ready result is None")
                .expect("poll ready result is an Error"),
            Poll::Pending => panic!("no ClientController request available"),
        };

        let result = match request {
            wlan_policy::ClientControllerRequest::StartClientConnections { responder } => {
                responder.send(response)
            }
            wlan_policy::ClientControllerRequest::StopClientConnections { responder } => {
                responder.send(response)
            }
            wlan_policy::ClientControllerRequest::Connect { responder, .. } => {
                responder.send(response)
            }
            _ => panic!("expecting a request that expects a RequestStatus"),
        };

        if result.is_err() {
            panic!("could not send request status");
        }
    }

    /// Creates a ClientStateSummary to provide as an update to listeners.
    fn create_state_summary(
        ssid: &str,
        state: wlan_policy::ConnectionState,
    ) -> wlan_policy::ClientStateSummary {
        let network_state = wlan_policy::NetworkState {
            id: Some(create_network_id(ssid)),
            state: Some(state),
            status: None,
        };
        wlan_policy::ClientStateSummary {
            state: Some(wlan_policy::WlanClientState::ConnectionsEnabled),
            networks: Some(vec![network_state]),
        }
    }

    /// Allows callers to send a response to SaveNetwork and RemoveNetwork calls.
    fn send_network_config_response(
        exec: &mut Executor,
        mut server: wlan_policy::ClientControllerRequestStream,
        success: bool,
    ) {
        let poll = exec.run_until_stalled(&mut server.next());
        let request = match poll {
            Poll::Ready(poll_ready) => poll_ready
                .expect("poll ready result is None")
                .expect("poll ready result is an Error"),
            Poll::Pending => panic!("no ClientController request available"),
        };

        let result = match request {
            wlan_policy::ClientControllerRequest::SaveNetwork { config: _, responder } => {
                if success {
                    responder.send(&mut Ok(()))
                } else {
                    responder.send(&mut Err(wlan_policy::NetworkConfigChangeError::GeneralError))
                }
            }
            wlan_policy::ClientControllerRequest::RemoveNetwork { config: _, responder } => {
                if success {
                    responder.send(&mut Ok(()))
                } else {
                    responder.send(&mut Err(wlan_policy::NetworkConfigChangeError::GeneralError))
                }
            }
            _ => panic!("expecting a request that optionally receives a NetworkConfigChangeError"),
        };

        if result.is_err() {
            panic!("could not send network config response");
        }
    }

    /// Creates a NetworkIdentifier for use in tests.
    fn create_network_id(ssid: &str) -> fidl_fuchsia_wlan_policy::NetworkIdentifier {
        wlan_policy::NetworkIdentifier {
            ssid: ssid.as_bytes().to_vec(),
            type_: wlan_policy::SecurityType::Wpa2,
        }
    }

    /// Creates a NetworkConfig for use in tests.
    fn create_network_config(ssid: &str) -> fidl_fuchsia_wlan_policy::NetworkConfig {
        wlan_policy::NetworkConfig { id: Some(create_network_id(ssid)), credential: None }
    }

    /// Creates a structure equivalent to what would be generated by the argument parsing to
    /// represent a NetworkIdentifier.
    fn create_network_id_arg(ssid: &str) -> opts::PolicyNetworkId {
        PolicyNetworkId { ssid: ssid.to_string(), security_type: SecurityTypeArg::Wpa2 }
    }

    /// Creates a structure equivalent to what would be generated by the argument parsing to
    /// represent a NetworkConfig.
    fn create_network_config_arg(ssid: &str) -> opts::PolicyNetworkConfig {
        PolicyNetworkConfig {
            ssid: ssid.to_string(),
            security_type: SecurityTypeArg::Wpa2,
            credential_type: CredentialTypeArg::Password,
            credential: Some("some_password_here".to_string()),
        }
    }

    /// Create a scan result to be sent as a response to a scan request.
    fn create_scan_result(ssid: &str) -> wlan_policy::ScanResult {
        wlan_policy::ScanResult {
            id: Some(create_network_id(ssid)),
            entries: None,
            compatibility: None,
        }
    }

    /// Respond to a ScanForNetworks request and provide an iterator so that tests can inject scan
    /// results.
    fn get_scan_result_iterator(
        exec: &mut Executor,
        mut server: wlan_policy::ClientControllerRequestStream,
    ) -> wlan_policy::ScanResultIteratorRequestStream {
        let poll = exec.run_until_stalled(&mut server.next());
        let request = match poll {
            Poll::Ready(poll_ready) => poll_ready
                .expect("poll ready result is None")
                .expect("poll ready result is an Error"),
            Poll::Pending => panic!("no ClientController request available"),
        };

        match request {
            wlan_policy::ClientControllerRequest::ScanForNetworks {
                iterator,
                control_handle: _,
            } => match iterator.into_stream() {
                Ok(stream) => stream,
                Err(e) => panic!("could not convert iterator into stream: {}", e),
            },
            _ => panic!("expecting a ScanForNetworks"),
        }
    }

    /// Sends scan results back to the client that is requesting a scan.
    fn send_scan_result(
        exec: &mut Executor,
        server: &mut wlan_policy::ScanResultIteratorRequestStream,
        mut scan_result: &mut Result<Vec<wlan_policy::ScanResult>, wlan_policy::ScanErrorCode>,
    ) {
        assert_variant!(
            exec.run_until_stalled(&mut server.next()),
            Poll::Ready(Some(Ok(wlan_policy::ScanResultIteratorRequest::GetNext {
                responder
            }))) => {
                match responder.send(&mut scan_result) {
                    Ok(()) => {}
                    Err(e) => panic!("failed to send scan result: {}", e),
                }
            }
        );
    }

    /// Responds to a GetSavedNetworks request and provide an iterator for sending back saved
    /// networks.
    fn get_saved_networks_iterator(
        exec: &mut Executor,
        mut server: wlan_policy::ClientControllerRequestStream,
    ) -> wlan_policy::NetworkConfigIteratorRequestStream {
        let poll = exec.run_until_stalled(&mut server.next());
        let request = match poll {
            Poll::Ready(poll_ready) => poll_ready
                .expect("poll ready result is None")
                .expect("poll ready result is an Error"),
            Poll::Pending => panic!("no ClientController request available"),
        };

        match request {
            wlan_policy::ClientControllerRequest::GetSavedNetworks {
                iterator,
                control_handle: _,
            } => match iterator.into_stream() {
                Ok(stream) => stream,
                Err(e) => panic!("could not convert iterator into stream: {}", e),
            },
            _ => panic!("expecting a ScanForNetworks"),
        }
    }

    /// Uses the provided iterator and send back saved networks.
    fn send_saved_networks(
        exec: &mut Executor,
        server: &mut wlan_policy::NetworkConfigIteratorRequestStream,
        saved_networks_response: Vec<wlan_policy::NetworkConfig>,
    ) {
        assert_variant!(
            exec.run_until_stalled(&mut server.next()),
            Poll::Ready(Some(Ok(wlan_policy::NetworkConfigIteratorRequest::GetNext {
                responder
            }))) => {
                match responder.send(&mut saved_networks_response.into_iter()) {
                    Ok(()) => {}
                    Err(e) => panic!("failed to send saved networks: {}", e),
                }
            }
        );
    }

    /// Tests the case where start client connections is called and the operation is successful.
    #[test]
    fn test_start_client_connections_success() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let mut test_values = test_setup();
        let fut = handle_start_client_connections(test_values.client_proxy);
        pin_mut!(fut);

        // Wait for the fidl request to go out to start client connections
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an acknowledgement
        send_request_status(
            &mut exec,
            &mut test_values.client_stream,
            fidl_wlan_common::RequestStatus::Acknowledged,
        );

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Ok(())));
    }

    /// Tests the case where starting client connections fails.
    #[test]
    fn test_start_client_connections_fail() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let mut test_values = test_setup();
        let fut = handle_start_client_connections(test_values.client_proxy);
        pin_mut!(fut);

        // Wait for the fidl request to go out to start client connections
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an acknowledgement
        send_request_status(
            &mut exec,
            &mut test_values.client_stream,
            fidl_wlan_common::RequestStatus::RejectedNotSupported,
        );

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Err(_)));
    }

    /// Tests the case where starting client connections is successful.
    #[test]
    fn test_stop_client_connections_success() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let mut test_values = test_setup();
        let fut = handle_stop_client_connections(test_values.client_proxy);
        pin_mut!(fut);

        // Wait for the fidl request to go out to stop client connections
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an acknowledgement
        send_request_status(
            &mut exec,
            &mut test_values.client_stream,
            fidl_wlan_common::RequestStatus::Acknowledged,
        );

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Ok(())));
    }

    /// Tests the case where starting client connections fails.
    #[test]
    fn test_stop_client_connections_fail() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let mut test_values = test_setup();
        let fut = handle_stop_client_connections(test_values.client_proxy);
        pin_mut!(fut);

        // Wait for the fidl request to go out to stop client connections
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an acknowledgement
        send_request_status(
            &mut exec,
            &mut test_values.client_stream,
            fidl_wlan_common::RequestStatus::RejectedNotSupported,
        );

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Err(_)));
    }

    /// Tests the case where a network is successfully saved.
    #[test]
    fn test_save_network_pass() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();
        let config = create_network_config_arg(TEST_SSID);
        let fut = handle_save_network(test_values.client_proxy, config);
        pin_mut!(fut);

        // Wait for the fidl request to go out to save a network
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Drop the remote channel indicating success
        send_network_config_response(&mut exec, test_values.client_stream, true);

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Ok(())));
    }

    /// Tests the case where a network configuration cannot be saved.
    #[test]
    fn test_save_network_fail() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();
        let config = create_network_config_arg(TEST_SSID);
        let fut = handle_save_network(test_values.client_proxy, config);
        pin_mut!(fut);

        // Wait for the fidl request to go out to save a network
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an error
        send_network_config_response(&mut exec, test_values.client_stream, false);

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Err(_)));
    }

    /// Tests the case where a network config can be successfully removed.
    #[test]
    fn test_remove_network_pass() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();
        let config = create_network_config_arg(TEST_SSID);
        let fut = handle_remove_network(test_values.client_proxy, config);
        pin_mut!(fut);

        // Wait for the fidl request to go out to remove a network
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Drop the remote channel indicating success
        send_network_config_response(&mut exec, test_values.client_stream, true);

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Ok(())));
    }

    /// Tests the case where network removal fails.
    #[test]
    fn test_remove_network_fail() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();
        let config = create_network_config_arg(TEST_SSID);
        let fut = handle_remove_network(test_values.client_proxy, config);
        pin_mut!(fut);

        // Wait for the fidl request to go out to remove a network
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an error
        send_network_config_response(&mut exec, test_values.client_stream, false);

        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Err(_)));
    }

    /// Tests the case where the client successfully connects.
    #[test]
    fn test_connect_pass() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let mut test_values = test_setup();

        // Start the connect routine.
        let config = create_network_id_arg(TEST_SSID);
        let fut = handle_connect(test_values.client_proxy, test_values.update_stream, config);
        pin_mut!(fut);

        // The function should now stall out waiting on the connect call to go out
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back a positive acknowledgement
        send_request_status(
            &mut exec,
            &mut test_values.client_stream,
            fidl_wlan_common::RequestStatus::Acknowledged,
        );

        // The client should now wait for events from the listener.  Send a Connecting update.
        assert!(exec.run_until_stalled(&mut fut).is_pending());
        let _ = test_values.update_proxy.on_client_state_update(create_state_summary(
            TEST_SSID,
            wlan_policy::ConnectionState::Connecting,
        ));

        // The client should stall and then wait for a Connected message.  Send that over.
        assert!(exec.run_until_stalled(&mut fut).is_pending());
        let _ = test_values.update_proxy.on_client_state_update(create_state_summary(
            TEST_SSID,
            wlan_policy::ConnectionState::Connected,
        ));

        // The connect process should complete after receiving the Connected status response
        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Ok(())));
    }

    /// Tests the case where a client fails to connect.
    #[test]
    fn test_connect_fail() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let mut test_values = test_setup();

        // Start the connect routine.
        let config = create_network_id_arg(TEST_SSID);
        let fut = handle_connect(test_values.client_proxy, test_values.update_stream, config);
        pin_mut!(fut);

        // The function should now stall out waiting on the connect call to go out
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back a positive acknowledgement
        send_request_status(
            &mut exec,
            &mut test_values.client_stream,
            fidl_wlan_common::RequestStatus::Acknowledged,
        );

        // The client should now wait for events from the listener
        assert!(exec.run_until_stalled(&mut fut).is_pending());
        let _ = test_values.update_proxy.on_client_state_update(create_state_summary(
            TEST_SSID,
            wlan_policy::ConnectionState::Failed,
        ));

        // The connect process should return an error after receiving a Failed status
        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Err(_)));
    }

    /// Tests the case where a scan is requested and results are sent back to the requester.
    #[test]
    fn test_scan_pass() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();

        let fut = handle_scan(test_values.client_proxy);
        pin_mut!(fut);

        // The function should now stall out waiting on the scan call to go out
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back a scan result
        let mut iterator = get_scan_result_iterator(&mut exec, test_values.client_stream);
        send_scan_result(&mut exec, &mut iterator, &mut Ok(vec![create_scan_result(TEST_SSID)]));

        // Process the scan result
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an empty scan result
        send_scan_result(&mut exec, &mut iterator, &mut Ok(Vec::new()));

        // Expect the scan process to complete
        assert_variant!(
            exec.run_until_stalled(&mut fut),
            Poll::Ready(Ok(result)) => {
                assert_eq!(result.len(), 1);
                assert_eq!(result[0].id.as_ref().unwrap().ssid, TEST_SSID.as_bytes().to_vec());
            }
        );
    }

    /// Tests the case where the scan cannot be performed.
    #[test]
    fn test_scan_fail() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();

        let fut = handle_scan(test_values.client_proxy);
        pin_mut!(fut);

        // The function should now stall out waiting on the scan call to go out
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back a scan error
        let mut iterator = get_scan_result_iterator(&mut exec, test_values.client_stream);
        send_scan_result(
            &mut exec,
            &mut iterator,
            &mut Err(wlan_policy::ScanErrorCode::GeneralError),
        );

        // Process the scan error
        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Ready(Err(_)));
    }

    /// Tests the ability to get saved networks from the client policy layer.
    #[test]
    fn test_get_saved_networks() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();

        let fut = handle_get_saved_networks(test_values.client_proxy);
        pin_mut!(fut);

        // The future should stall out waiting on the get saved networks request
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back a saved networks response
        let mut iterator = get_saved_networks_iterator(&mut exec, test_values.client_stream);
        send_saved_networks(&mut exec, &mut iterator, vec![create_network_config(TEST_SSID)]);
        assert!(exec.run_until_stalled(&mut fut).is_pending());

        // Send back an empty set of configs to indicate that the process is complete
        send_saved_networks(&mut exec, &mut iterator, vec![]);

        // Verify that the saved networks response was recorded properly
        assert_variant!(
            exec.run_until_stalled(&mut fut),
            Poll::Ready(Ok(result)) => {
                assert_eq!(result.len(), 1);
                assert_eq!(result[0].id.as_ref().unwrap().ssid, TEST_SSID.as_bytes().to_vec());
            }
        );
    }

    /// Tests to ensure that the listening loop continues to be active after receiving client state updates.
    #[test]
    fn test_listen() {
        let mut exec = Executor::new().expect("failed to create an executor");
        let test_values = test_setup();

        let fut = handle_listen(test_values.update_stream);
        pin_mut!(fut);

        // Listen should stall waiting for updates
        assert!(exec.run_until_stalled(&mut fut).is_pending());
        let _ = test_values.update_proxy.on_client_state_update(create_state_summary(
            TEST_SSID,
            wlan_policy::ConnectionState::Connecting,
        ));

        // Listen should process the message and stall again
        assert!(exec.run_until_stalled(&mut fut).is_pending());
        let _ = test_values.update_proxy.on_client_state_update(create_state_summary(
            TEST_SSID,
            wlan_policy::ConnectionState::Connected,
        ));

        // Listener future should continue to run but stall waiting for more updates
        assert_variant!(exec.run_until_stalled(&mut fut), Poll::Pending);
    }
}