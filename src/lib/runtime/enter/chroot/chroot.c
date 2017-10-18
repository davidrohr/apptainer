/* 
 * Copyright (c) 2017, SingularityWare, LLC. All rights reserved.
 *
 * Copyright (c) 2015-2017, Gregory M. Kurtzer. All rights reserved.
 * 
 * Copyright (c) 2016-2017, The Regents of the University of California,
 * through Lawrence Berkeley National Laboratory (subject to receipt of any
 * required approvals from the U.S. Dept. of Energy).  All rights reserved.
 * 
 * This software is licensed under a customized 3-clause BSD license.  Please
 * consult LICENSE file distributed with the sources of this project regarding
 * your rights to use or distribute this software.
 * 
 * NOTICE.  This Software was developed under funding from the U.S. Department of
 * Energy and the U.S. Government consequently retains certain rights. As such,
 * the U.S. Government has been granted for itself and others acting on its
 * behalf a paid-up, nonexclusive, irrevocable, worldwide license in the Software
 * to reproduce, distribute copies to the public, prepare derivative works, and
 * perform publicly and display publicly, and to permit other to do so. 
 * 
*/

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/mount.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/syscall.h>
#include <unistd.h>
#include <stdlib.h>

#include "config.h"
#include "util/file.h"
#include "util/util.h"
#include "util/message.h"
#include "util/privilege.h"
#include "util/registry.h"
#include "util/config_parser.h"

#include "../../runtime.h"

int pivot_root(const char *new_root, const char *put_old) {
    return syscall(__NR_pivot_root, new_root, put_old);
}

int _singularity_runtime_enter_chroot(void) {
    char *container_dir = CONTAINER_FINALDIR;

    singularity_message(VERBOSE, "Entering container file system root: %s\n", container_dir);

    singularity_priv_escalate();
    if ( singularity_registry_get("DAEMON_JOIN") == NULL ) {
        if ( chdir(container_dir) < 0 ) {
            singularity_message(ERROR, "Could not chdir to file system root %s: %s\n", container_dir, strerror(errno));
            ABORT(1);
        }
        if ( pivot_root(".", "bin") < 0 ) {
            singularity_message(ERROR, "Changing root filesystem failed\n");
            ABORT(255);
        }
        if ( chroot(".") < 0 ) { // Flawfinder: ignore (yep, yep, yep... we know!)
            singularity_message(ERROR, "failed chroot to container at: %s\n", container_dir);
            ABORT(255);
        }
        if ( umount2("bin", MNT_DETACH) < 0 ) {
            singularity_message(ERROR, "Changing root filesystem failed\n");
            ABORT(255);
        }
    } else {
        if ( chroot("/proc/1/root") < 0 ) { // Flawfinder: ignore (yep, yep, yep... we know!)
            singularity_message(ERROR, "failed chroot to container at: %s\n", container_dir);
            ABORT(255);
        }
    }
    singularity_priv_drop();

    singularity_message(DEBUG, "Changing dir to '/' within the new root\n");
    if ( chdir("/") < 0 ) {
        singularity_message(ERROR, "Could not chdir after chroot to /: %s\n", strerror(errno));
        ABORT(1);
    }

    return(0);
}


