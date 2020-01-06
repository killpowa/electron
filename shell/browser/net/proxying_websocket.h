// Copyright 2018 The Chromium Authors. All rights reserved.
// Use of this source code is governed by a BSD-style license that can be
// found in the LICENSE file.

#ifndef EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_PROXYING_WEBSOCKET_H_
#define EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_PROXYING_WEBSOCKET_H_

#include <string>
#include <vector>

#include "base/callback.h"
#include "base/macros.h"
#include "base/memory/ref_counted.h"
#include "base/memory/weak_ptr.h"
#include "base/optional.h"
#include "components/keyed_service/core/keyed_service_shutdown_notifier.h"
#include "content/public/browser/content_browser_client.h"
#include "extensions/browser/api/web_request/web_request_info.h"
#include "mojo/public/cpp/bindings/pending_receiver.h"
#include "mojo/public/cpp/bindings/receiver.h"
#include "mojo/public/cpp/bindings/remote.h"
#include "net/base/network_delegate.h"
#include "services/network/public/cpp/resource_request.h"
#include "services/network/public/cpp/resource_response.h"
#include "services/network/public/mojom/network_context.mojom.h"
#include "services/network/public/mojom/websocket.mojom.h"
#include "shell/browser/api/atom_web_request_api.h"
#include "url/gurl.h"
#include "url/origin.h"

namespace electron {

// A ProxyingWebSocket proxies a WebSocket connection and dispatches
// WebRequest API events.
class ProxyingWebSocket : public network::mojom::WebSocketHandshakeClient,
                          public network::mojom::AuthenticationHandler,
                          public network::mojom::TrustedHeaderClient {
 public:
  using WebSocketFactory = content::ContentBrowserClient::WebSocketFactory;

  ProxyingWebSocket(
      WebRequestAPI* web_request_api,
      WebSocketFactory factory,
      const network::ResourceRequest& request,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      bool has_extra_headers,
      int process_id,
      int render_frame_id,
      content::BrowserContext* browser_context);
  ~ProxyingWebSocket() override;

  void StartProxy();

  WebRequestAPI* web_request_api() { return web_request_api_; }

  // network::mojom::WebSocketHandshakeClient methods:
  void OnOpeningHandshakeStarted(
      network::mojom::WebSocketHandshakeRequestPtr request) override;
  void OnResponseReceived(
      network::mojom::WebSocketHandshakeResponsePtr response) override;
  void OnConnectionEstablished(
      mojo::PendingRemote<network::mojom::WebSocket> websocket,
      mojo::PendingReceiver<network::mojom::WebSocketClient> client_receiver,
      const std::string& selected_protocol,
      const std::string& extensions,
      mojo::ScopedDataPipeConsumerHandle readable) override;

  // network::mojom::AuthenticationHandler method:
  void OnAuthRequired(const net::AuthChallengeInfo& auth_info,
                      const scoped_refptr<net::HttpResponseHeaders>& headers,
                      const net::IPEndPoint& remote_endpoint,
                      OnAuthRequiredCallback callback) override;

  // network::mojom::TrustedHeaderClient methods:
  void OnBeforeSendHeaders(const net::HttpRequestHeaders& headers,
                           OnBeforeSendHeadersCallback callback) override;
  void OnHeadersReceived(const std::string& headers,
                         OnHeadersReceivedCallback callback) override;

  static void StartProxying(
      WebRequestAPI* web_request_api,
      WebSocketFactory factory,
      const GURL& url,
      const GURL& site_for_cookies,
      const base::Optional<std::string>& user_agent,
      mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
          handshake_client,
      bool has_extra_headers,
      int process_id,
      int render_frame_id,
      const url::Origin& origin,
      content::BrowserContext* browser_context);

 private:
  WebRequestAPI* web_request_api_;

  void OnBeforeRequestComplete(int error_code);
  void OnBeforeSendHeadersComplete(const std::set<std::string>& removed_headers,
                                   const std::set<std::string>& set_headers,
                                   int error_code);
  void ContinueToStartRequest(int error_code);
  void OnHeadersReceivedComplete(int error_code);
  void ContinueToHeadersReceived();
  void OnAuthRequiredComplete(net::NetworkDelegate::AuthRequiredResponse rv);
  void OnHeadersReceivedCompleteForAuth(const net::AuthChallengeInfo& auth_info,
                                        int rv);

  void PauseIncomingMethodCallProcessing();
  void ResumeIncomingMethodCallProcessing();
  void OnError(int result);
  void OnMojoConnectionError(uint32_t custom_reason,
                             const std::string& description);

  WebSocketFactory factory_;
  network::ResourceRequest request_;
  mojo::Remote<network::mojom::WebSocketHandshakeClient>
      forwarding_handshake_client_;
  mojo::PendingRemote<network::mojom::WebSocketHandshakeClient>
      handshake_client_;
  net::HttpRequestHeaders request_headers_;
  bool has_extra_headers_;

  base::Optional<extensions::WebRequestInfo> info_;
  mojo::Receiver<network::mojom::WebSocketHandshakeClient>
      receiver_as_handshake_client_{this};
  mojo::Receiver<network::mojom::AuthenticationHandler>
      receiver_as_auth_handler_{this};
  mojo::Receiver<network::mojom::TrustedHeaderClient>
      receiver_as_header_client_{this};
  network::ResourceResponseHead response_;
  net::AuthCredentials auth_credentials_;
  OnAuthRequiredCallback auth_required_callback_;
  scoped_refptr<net::HttpResponseHeaders> override_headers_;
  std::vector<network::mojom::HttpHeaderPtr> additional_headers_;

  OnBeforeSendHeadersCallback on_before_send_headers_callback_;
  OnHeadersReceivedCallback on_headers_received_callback_;

  GURL redirect_url_;
  bool is_done_ = false;
  bool waiting_for_header_client_headers_received_ = false;

  // Notifies the proxy that the browser context has been shutdown.
  std::unique_ptr<KeyedServiceShutdownNotifier::Subscription>
      shutdown_notifier_;

  base::WeakPtrFactory<ProxyingWebSocket> weak_factory_{this};
  DISALLOW_COPY_AND_ASSIGN(ProxyingWebSocket);
};

}  // namespace electron

#endif  // EXTENSIONS_BROWSER_API_WEB_REQUEST_WEB_REQUEST_PROXYING_WEBSOCKET_H_
