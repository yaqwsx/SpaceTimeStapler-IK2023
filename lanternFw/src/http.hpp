#include <esp_http_client.h>
#include <esp_log.h>
#include <string>
#include <memory>
#include <iostream>
#include <cJSON.h>


template < typename F >
int httpLibCallbackInvoker( esp_http_client_event_t *evt ) {
    F& function = *reinterpret_cast< F * >( evt->user_data );
    return function( evt );
}

/**
 * Builds a callback which reads only data out of http request and processes it
 * via f
 */
template < typename F >
auto makeHttpDataCallback( F f ) {
    return [=]( esp_http_client_event_t *evt ) {
        switch(evt->event_id) {
            case HTTP_EVENT_ON_DATA:
                f( static_cast< char *>( evt->data ), evt->data_len );
                break;
            default:
                break;
        }
        return ESP_OK;
    };
}

struct JSONDeleter {
    void operator()( cJSON* j ) {
        if (j)
            cJSON_Delete( j );
    }
};

using JsonP = std::unique_ptr< cJSON, JSONDeleter >;

inline JsonP getJson( const std::string& url ) {
    std::string body;
    auto httpCallback = makeHttpDataCallback( [&]( char *data, int len ) {
        body.append( data, len );
    });

    std::cout << "URL: " << url << "\n";

    esp_http_client_config_t config {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_GET;
    config.user_data = &httpCallback;
    config.event_handler = httpLibCallbackInvoker< decltype( httpCallback ) >;

    esp_http_client_handle_t client  = esp_http_client_init( &config );
    int result = esp_http_client_perform( client );
    if ( result == ESP_FAIL )
        return JsonP( nullptr );
    int status = esp_http_client_get_status_code( client );
    esp_http_client_cleanup( client );

    if ( status != 200 )
        std::cout << "Error occured\n";

    std::cout << "Body: " << body << "\n";

    cJSON *json = cJSON_Parse( body.c_str() );
    return JsonP( json );
}

inline JsonP postJson( const std::string& url, const std::string& data ) {
    std::string body;
    auto httpCallback = makeHttpDataCallback( [&]( char *data, int len ) {
        body.append( data, len );
    });

    std::cout << "URL: " << url << "\n";
    std::cout << "Data: " << data << "\n";

    esp_http_client_config_t config {};
    config.url = url.c_str();
    config.method = HTTP_METHOD_POST;
    config.user_data = &httpCallback;
    config.event_handler = httpLibCallbackInvoker< decltype( httpCallback ) >;

    esp_http_client_handle_t client  = esp_http_client_init( &config );
    esp_http_client_set_header( client, "Content-Type", "application/json; charset=utf-8" );
    esp_http_client_set_post_field( client, data.c_str(), data.length() );
    int result = esp_http_client_perform( client );
    if ( result == ESP_FAIL )
        return JsonP( nullptr );
    int status = esp_http_client_get_status_code( client );
    esp_http_client_cleanup( client );

    if ( status != 200 )
        std::cout << "Error occured\n";

    std::cout << "Body: " << body << "\n";

    cJSON *json = cJSON_Parse( body.c_str() );
    return JsonP( json );
}
