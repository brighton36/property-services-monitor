CXX=g++
CFLAGS=-I.


property-services-monitor: property-services-monitor.o 
	     $(CXX) -o property-services-monitor property-services-monitor.o -lyaml-cpp

.PHONY: clean

clean:
	rm -f *.o property-services-monitor
