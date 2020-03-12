# property-services-monitor
This is a lightweight tool to test services and servers on a property, and e-mail
an address with the results of those tests. 

![Sample Report e-mail output](sample_report_screenshot.png)

This tool is written in modern C++, and designed to be portable, for use on 
raspberri pi's, or similar systems.

**NOTE: Some (most) systems require this executable to run as suid root, in order
to perform ping tests. If you're seeing an "I/O error exception" in your reports, 
this is why you're receiving that error message.**

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
sudo apt install build-essential libfmt-dev libyaml-cpp-dev libpoco-dev nlohmann-json3-dev doctest-dev
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

## CHANGELOG

#### [Version 0.2](https://github.com/brighton36/property-services-monitor/releases/tag/0.2) (Date-todo)
- Added "Blue Iris" Service monitoring module.
- Incorporated DVR image attachment support and presentation to the report email.
- Implemented preliminary "make test" target with doctest.

#### [Version 0.1](https://github.com/brighton36/property-services-monitor/releases/tag/0.1) (2020-02-29)
- First Production Release.

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
 * type        (required) The type of service being tested. The type must be one of the
                          following supported types: "blueiris", "ping", "web".

                          Depending on the value of this parameter, additional sequence
                          options may be available. in this service's map section. What
                          follows are type-specific parameters.

For "blueiris" service types:
 * username         (required) A username for use in logging into the blue iris web service.
 * password         (required) A password for use in logging into the blue iris web service.
 * proto            (optional) Either "http" or "https". Defaults to "http".
 * port             (optional) The port number of the http service. Defaults to 80 (http) or
                             443 (https), depending on the proto value.
 * capture_camera   (optional) Which camera(s) to capture alert images from, for inclusion
                               in our report. Defaults to "Index" (Include All cameras).
 * capture_from     (optional) A human readable DATETIME, indicating what time and day will
                               start our alert image capturing. This is used to download
                               and attach images to our report. See the notes on DATETIME
                               formatting further below. Defaults to "11:00P the day before".
 * capture_to       (optional) A human readable DATETIME, indicating what time and day will
                               end our alert image capturing. This is used to download
                               and attach images to our report. See the notes on DATETIME
                               formatting further below. Defaults to "Today at 4:00A".
 * max_warnings     (optional) Warnings are returned by the "status" command, to the 
                               blue iris server. This is the count threshold, above which
                               we trigger failure. Defaults to 0.
 * min_uptime       (optional) The minimum acceptable system uptime for this server. This
                               parameter is expected to be provided in the DURATION format.
                               For more details on the DURATION format, see below. Defaults
                               to "1 day".
 * min_percent_free (optional) The minimum amount of space required on the system hard disk(s)
                               in order to pass this check. This test is applied to every drive
                               installed in the system. Defaults to "5%".

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

The DATETIME format:
  This string format supports multiple descriptions of relative dates and times, for use
  in your configurations. Some examples include : "Yesterday at 11:00a", "2 hours ago"
  "11:00 P, 2 days prior", "Today at 1:05 P", and even
  "2 days, 1 hour, 10 minutes back...". All times are relative to "now", the time at.
  which the program is being run.

The DURATION format:
  This string indicates a number of seconds, for use in your configurations. Some 
  examples include: "30 seconds", "2 hours", "20 days", "1 month", and even 
  "3 weeks, 2 hours, 30 minutes, 10 seconds".

For more information about this program, see the github repo at:
  https://github.com/brighton36/property-services-monitor/
```
