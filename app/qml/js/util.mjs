
const BYTE_UNITS = [
        'B',
        'kB',
        'MB',
        'GB',
        'TB',
        'PB',
        'EB',
        'ZB',
        'YB',
];

export function prettyBytes(number, maxFrac=0) {
    const UNITS = BYTE_UNITS;
        const exponent = number < 1 
        ? 0 
        : Math.min(Math.floor(Math.log(number) / Math.log(1024)), UNITS.length - 1);
    const unit = UNITS[exponent];
    const prefix = number < 0 ? '-' : '';

        const num_str = (number / 1024 ** exponent).toFixed(maxFrac);
    return `${prefix}${num_str} ${unit}`; 
}
