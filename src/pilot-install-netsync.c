/*
 * $Id$ 
 *
 * pilot-install-netsync.c:  Palm Network Information Installer
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the
 * Free Software Foundation; either version 2 of the License, or (at your
 * option) any later version.
 *
 * This program is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General
 * Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <netinet/in.h>
#include <netdb.h>

#include "pi-dlp.h"
#include "pi-header.h"
#include "pi-userland.h"

poptContext po;

int main(int argc, char *argv[])
{
	int 	enable	= -1,
		sd 	= -1,
		po_err	= -1;

	const char
		*hostname 	= NULL,
		*address 	= NULL,
		*netmask 	= NULL;

	const struct poptOption options[] = {
		USERLAND_RESERVED_OPTIONS
		{ "enable",  'e', POPT_ARG_VAL,    &enable, 1, "Enables LANSync on the Palm"},
		{ "disable", 'd', POPT_ARG_VAL,    &enable, 0, "Disable the LANSync setting on the Palm"},
		{ "name",    'n', POPT_ARG_STRING, &hostname, 0, "The hostname of the remote machine you sync with", "<name>"},
		{ "address", 'a', POPT_ARG_STRING, &address, 0, "IP address of the remote machine you connect to", "<address>"},
		{ "mask",    'm', POPT_ARG_STRING, &netmask, 0, "Subnet mask of the network your Palm is on", "<netmask>"},
		POPT_TABLEEND
	};

	struct 	NetSyncInfo 	Net;
	struct in_addr addr;

	po = poptGetContext("pilot-install-netsync", argc, (const char **) argv, options, 0);
	poptSetOtherOptionHelp(po," [-p port] <netsync options>\n\n"
		"   Assigns your Palm device NetSync information\n\n"
		"   Example:\n"
		"      -p usb: -e -n \"NEPTUNE\" -a 10.0.1.10 -m 255.255.255.0\n\n");

	if (argc < 2) {
		poptPrintUsage(po,stderr,0);
		return 1;
	}

	while ((po_err = poptGetNextOpt(po)) >= 0) {
	}
	if (po_err < -1) plu_badoption(po,po_err);

	if (address && !inet_pton(AF_INET, address, &addr)) {
		fprintf(stderr,"   ERROR: The address you supplied, '%s' is in invalid.\n"
			"      Please supply a dotted quad, such as 1.2.3.4\n\n", address);
		return 1;
	}

	/* Auto-resolve: if address given but no hostname, do reverse DNS */
	if (address && !hostname) {
		struct sockaddr_in sa;
		static char resolved_host[NI_MAXHOST];

		memset(&sa, 0, sizeof(sa));
		sa.sin_family = AF_INET;
		inet_pton(AF_INET, address, &sa.sin_addr);

		if (getnameinfo((struct sockaddr *)&sa, sizeof(sa),
				resolved_host, sizeof(resolved_host),
				NULL, 0, NI_NAMEREQD) == 0) {
			hostname = resolved_host;
		}
	}

	/* Auto-resolve: if hostname given but no address, do forward DNS */
	if (hostname && !address) {
		struct addrinfo hints, *res;
		static char resolved_addr[INET_ADDRSTRLEN];

		memset(&hints, 0, sizeof(hints));
		hints.ai_family = AF_INET;

		if (getaddrinfo(hostname, NULL, &hints, &res) == 0) {
			struct sockaddr_in *sa = (struct sockaddr_in *)res->ai_addr;
			if (inet_ntop(AF_INET, &sa->sin_addr,
				      resolved_addr, sizeof(resolved_addr))) {
				address = resolved_addr;
			}
			freeaddrinfo(res);
		}
	}

	if (netmask && !inet_pton(AF_INET, netmask, &addr)) {
		fprintf(stderr,"   ERROR: The netmask you supplied, '%s' is in invalid.\n"
			"      Please supply a dotted quad, such as 255.255.255.0\n\n", netmask);
		return 1;
	}

	sd = plu_connect();
	if (sd < 0)
		goto error;

	if (dlp_OpenConduit(sd) < 0)
		goto error_close;

	/* Read and write the LANSync data to the Palm device */
	if (dlp_ReadNetSyncInfo(sd, &Net) < 0)
		goto error_close;

	if (enable != -1)
		Net.lanSync = enable;

	if (!plu_quiet) {
		printf("   LANSync....: %sabled\n", (Net.lanSync == 1 ? "En" : "Dis"));
		fflush(stdout);
	}

	if (hostname) {
		strncpy(Net.hostName, hostname, sizeof(Net.hostName) - 1);
		Net.hostName[sizeof(Net.hostName) - 1] = '\0';
	}

	if (address) {
		strncpy(Net.hostAddress, address, sizeof(Net.hostAddress) - 1);
		Net.hostAddress[sizeof(Net.hostAddress) - 1] = '\0';
	}

	if (netmask) {
		strncpy(Net.hostSubnetMask, netmask,
			sizeof(Net.hostSubnetMask) - 1);
		Net.hostSubnetMask[sizeof(Net.hostSubnetMask) - 1] = '\0';
	}

	if (!plu_quiet) {
		printf("   Hostname...: %s\n", Net.hostName);
		printf("   IP Address.: %s\n", Net.hostAddress);
		printf("   Netmask....: %s\n\n", Net.hostSubnetMask);
	}

	if (dlp_WriteNetSyncInfo(sd, &Net) < 0)
		goto error_close;

	if (dlp_AddSyncLogEntry(sd, "pilot-install-netsync, exited normally.\n"
				"Thank you for using pilot-link.\n") < 0)
		goto error_close;

	if (dlp_EndOfSync(sd, 0) < 0)
		goto error_close;

	if (pi_close(sd) < 0)
		goto error;

	return 0;

error_close:
	pi_close(sd);

error:
	return -1;
}

/* vi: set ts=8 sw=4 sts=4 noexpandtab: cin */
/* ex: set tabstop=4 expandtab: */
/* Local Variables: */
/* indent-tabs-mode: t */
/* c-basic-offset: 8 */
/* End: */
