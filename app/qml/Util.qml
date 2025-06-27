pragma Singleton
import QtQuick

import Qcm.App as QA
import Qcm.Material as MD

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

    function loopModeIcon(loopMode: int): var {
        switch (loopMode) {
        case QA.Enum.SingleLoop:
            return MD.Token.icon.repeat_one;
        case QA.Enum.ListLoop:
            return MD.Token.icon.loop;
        case QA.Enum.ShuffleLoop:
            return MD.Token.icon.shuffle;
        case QA.Enum.NoneLoop:
        default:
            return MD.Token.icon.trending_flat;
        }
    }

    function displayModeIcon(mode) {
        switch (mode) {
        case QA.Enum.DGrid:
        case QA.Enum.DCardGrid:
            return MD.Token.icon.grid_view;
        case QA.Enum.DList:
        default:
            return MD.Token.icon.list_alt;
        }
    }
}
