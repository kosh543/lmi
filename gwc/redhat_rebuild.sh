#!/bin/sh

cd /srv/cache_for_lmi/logs || printf 'Cannot cd\n'
wget -N -nv https://github.com/vadz/lmi/raw/master/lmi_setup_00.sh
chmod +x lmi_setup_00.sh
# It would be undesirable for root to own the output file:
# shellcheck disable=SC2024
sudo ./lmi_setup_00.sh >log_"$(date -u +"%Y%m%dT%H%MZ")" 2>&1
