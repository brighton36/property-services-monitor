# property-services-monitor
This is a lightweight tool to test services and servers on a property, and e-mail
an address with the results of those tests. 

![Sample Report e-mail output](sample_report_screenshot.png)

This tool is written in modern C++, and designed to be portable, for use on 
raspberri pi's, or similar systems.

**NOTE: Some (most) systems require this executable to run as suid root, in order
to perform ping tests. If you're seeing an "I/O Error" in your reports, this is
why you're receiving that error message.**

## Installation
The supplied 0.1 release contains compiled deb packages for amd64 and arm6+ 
platforms. 

To install on DietPi (presumably Raspbian as well) distributions, be sure to install
the dependencies: 
```
  apt install -f libpococrypto60 libpocofoundation60 libpoconet60 libpoconetssl60 libyaml-cpp0.6 libexpat1 libpocojson60 libpocoutil60 libpocoxml60
```
before installing the supplied package:
```
  dpkg -i propertyservicesmonitor_0.1-1_armhf.deb
```
Once installed, the property-services-monitor is intended to be run via a scheduled
cronjob. There is one expected parameter, a yaml file path, with site configuration 
details. 

## Compilation
If compiling on Debian systems, the following packages are required:
```
sudo apt install build-essential libfmt-dev libyaml-cpp-dev libpoco-dev
```
Compilation is as simple as :
```
make && make install
```
If you wish to create debian packages for additional architectures or further 
deployment, a "make package" target is included.

## Sample Config File
Included in this repository is a (sample, minimal sample_config.yml)[sample_config.yml] for getting started.

## Notes on the e-mail output template format
The email formatting templates can be specified in the config.yml. By default, 
the templates are automatically set to notify.html.inja and notify.plain.inja 
See these included templates for an example of how to write your own. Templates 
use the [inja library](https://pantor.github.io/inja/) for constructing plain and 
html output.

Note that some template adjustments can be performed without having to alter
or create new templates. To quickly adjust the default template, add the 
following parameters to your config.yml:
```
notification:
  #...
  parameters:
    thumbnail: /home/user/images/property-exterior.jpg
    body_color: #aaaaaa
    #...
```
To see what parameters are supported in the default template, search the included
notify.html.inja and notify_body.html.inja. 
Note: 
 - These parameters also submitted to the notify.plain.inja before rendering.
 - There are no restrictions on what key and value pairs you delineate in this
   'parameters' section. If you wish to add additional parameters for use in your
   own templates, just add them to this section of the config, and they will be
   mirrored onto the template at the time of rendering.
 - Relative Image paths are expected to be relative to the inja file itself.

## Program help Output
Additional details on the types of services that are supported, and what options
they support testing, can be found in the --help output of the program. 
```
Usage: ./property-services-monitor [-o] [FILE]
A lightweight service availability checking tool.

-o           Output a terse summary of the notification contents to STDOUT.
--help       Display this help and exit.

The supplied FILE is expected to be a yaml-formatted service monitor definition file.
(See https://en.wikipedia.org/wiki/YAML for details on the YAML file format.)

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
```
