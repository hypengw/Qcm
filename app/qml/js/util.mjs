
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

export function pretty_bytes(number, maxFrac = 0) {
    const UNITS = BYTE_UNITS;
    const exponent = number < 1
        ? 0
        : Math.min(Math.floor(Math.log(number) / Math.log(1024)), UNITS.length - 1);
    const unit = UNITS[exponent];
    const prefix = number < 0 ? '-' : '';

    const num_str = (number / 1024 ** exponent).toFixed(maxFrac);
    return `${prefix}${num_str} ${unit}`;
}

export function array_split(arr, count) {
    if (!arr.length) return [];
    const header = arr.slice(0, count);
    return [header].concat(array_split(arr.slice(count), count));
}

export function bound_img_size(x) {
    return 30 * (1 << Math.ceil(Math.log2( x / 30 )));
}