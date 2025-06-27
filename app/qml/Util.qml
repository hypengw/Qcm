pragma Singleton
import QtQuick

import Qcm.App as QA

QA.UtilCpp {
    readonly property list<string> byte_units: ['B', 'kB', 'MB', 'GB', 'TB', 'PB', 'EB', 'ZB', 'YB',]

    function pretty_bytes(number, maxFrac = 0) {
        const UNITS = byte_units;
        const exponent = number < 1 ? 0 : Math.min(Math.floor(Math.log(number) / Math.log(1024)), UNITS.length - 1);
        const unit = UNITS[exponent];
        const prefix = number < 0 ? '-' : '';

        const num_str = (number / 1024 ** exponent).toFixed(maxFrac);
        return `${prefix}${num_str} ${unit}`;
    }
}
