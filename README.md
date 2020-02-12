# property-services-monitor
A lightweight service availability checking tool.

Usage: ./property-services-monitor [config.yml]

The supplied argument is expected to be a yaml-formatted service monitor definition file.
(See https://en.wikipedia.org/wiki/YAML for details on the YAML file format.)

The following sections and parameters are supported in your supplied config file.

At the root of the config file, two parameters are required: "notification" and "hosts".

The "notification" (map) supports the following parameters:
 * to               (required) A valid SMTP address to which the notification will be delivered.
 * from             (required) A valid SMTP address from which the notification will be address.
 * subject          (required) The Subject line of the e-mail notification.
 * host             (required) The fqdn or ip address of the smtp server to use for relaying.
 * template_html    (optional) A relative or absolute path to an inja template, used 
                               for constructing the email html body
 * template_plain   (optional) A relative or absolute path to an inja template, used 
                               for constructing the email plain text body
 * template_subject (optional) An inja string used to format the smtp subject line
 * proto            (optional) Either "plain" or "ssl". Defaults to "plain".
 * port             (optional) The port number of the smtp relay server. Defaults to 25 (plain)
                               or 465 (ssl), depending on the "proto" value. 
 * username         (optional) The login credential for the smtp relay server.
 * password         (optional) The password credential for the smtp relay server.
 * parameters       (optional) A key to value (map). These parameters are passed to the template
                               files, as specified. See the included template files for what values
                               are supported here by the shipped templates. Feel free to add any
                               values of your own.

The "hosts" (sequence) is expected to provide an itemization of the systems being monitored.
Each host item in the sequence is itself a (map). 

The format of each host's (map) is as follows:
 * address     (required) The FQDN or IP address of the host.
 * label       (required) The human-readable moniker of this host. Used in the output report.
 * description (required) A description of the host, for use in the output report.
 * services    (required) A (sequence) of services to test, that are expected to be running on
                          this host. See below for details.

The "services" (sequence) is expected to provide an itemization of the service being tested
for availability. Each service in the sequence is itself a (map).

The format of service's (map) is as follows:
 * type           (required) The type of service being tested. The type must be one of the
                             following supported types: "ping", "web".

                             Depending on the value of this parameter, additional sequence
                             options may be available. in this service's map section. What
                             follows are type-specific parameters.

 For "ping" service types:
 * tries          (optional) The number of pings to attempt. Defaults to 5.
 * success_over   (optional) The number of pings, over which we are successful. Defaults to 4.

 For "web" service types:
 * proto          (optional) Either "http" or "https". Defaults to "http".
 * port           (optional) The port number of the http service. Defaults to 80 (http) or
                             443 (https), depending on the proto value.
 * path           (optional) The resource path being request from the http server. Defaults
                             to "/".
 * status_equals  (optional) The expected HTTP status code, to be received from the server.
                             Defaults to 200.
 * ensure_match   (optional) A regular expression to be found in the return content body.
                             This regex is expected to be in the C++ regex format (no /'s).

For more information about this program, see the github repo at:
  https://github.com/brighton36/property-services-monitor/
