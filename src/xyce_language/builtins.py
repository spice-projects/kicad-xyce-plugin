from __future__ import annotations

from collections.abc import Callable

import numpy as np


QspiceValue = np.ndarray

BuiltinCallable = Callable[[list[QspiceValue]], QspiceValue]


def _expect_arity(name: str, args: list[QspiceValue], count: int) -> None:
    # validate the exact builtin arity
    if len(args) != count:
        raise ValueError(f"Function {name!r} expects {count} arguments, got {len(args)}")


def _expect_min_arity(name: str, args: list[QspiceValue], count: int) -> None:
    # validate the minimum builtin arity
    if len(args) < count:
        raise ValueError(f"Function {name!r} expects at least {count} arguments, got {len(args)}")


def _reduce_min(args: list[QspiceValue]) -> QspiceValue:
    # seed the reduction with the first argument
    result = args[0]
    # fold the remaining arguments into the running minimum
    for arg in args[1:]:
        result = np.minimum(result, arg)
    # exit
    return result


def _reduce_max(args: list[QspiceValue]) -> QspiceValue:
    # seed the reduction with the first argument
    result = args[0]
    # fold the remaining arguments into the running maximum
    for arg in args[1:]:
        result = np.maximum(result, arg)
    # exit
    return result


def _builtin_abs(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("abs", args, 1)
    return np.abs(args[0])


def _builtin_sqrt(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("sqrt", args, 1)
    return np.sqrt(args[0])


def _builtin_log(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("log", args, 1)
    return np.log(args[0])


def _builtin_log10(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("log10", args, 1)
    return np.log10(args[0])


def _builtin_ln(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("ln", args, 1)
    return np.log(args[0])


def _builtin_db(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("db", args, 1)
    return 20.0 * np.log10(np.abs(args[0]))


def _builtin_real(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("real", args, 1)
    return np.real(args[0])


def _builtin_imag(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("imag", args, 1)
    return np.imag(args[0])


def _builtin_angle(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("angle", args, 1)
    return np.angle(args[0], deg=True)


def _builtin_mag(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("mag", args, 1)
    return np.abs(args[0])


def _builtin_sin(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("sin", args, 1)
    return np.sin(args[0])


def _builtin_cos(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("cos", args, 1)
    return np.cos(args[0])


def _builtin_tan(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("tan", args, 1)
    return np.tan(args[0])


def _builtin_asin(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("asin", args, 1)
    return np.arcsin(args[0])


def _builtin_acos(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("acos", args, 1)
    return np.arccos(args[0])


def _builtin_atan(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("atan", args, 1)
    return np.arctan(args[0])


def _builtin_atan2(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("atan2", args, 2)
    return np.arctan2(np.real(args[0]), np.real(args[1]))


def _builtin_sinh(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("sinh", args, 1)
    return np.sinh(args[0])


def _builtin_cosh(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("cosh", args, 1)
    return np.cosh(args[0])


def _builtin_tanh(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("tanh", args, 1)
    return np.tanh(args[0])


def _builtin_exp(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("exp", args, 1)
    return np.exp(args[0])


def _builtin_conj(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("conj", args, 1)
    return np.conj(args[0])


def _builtin_sqr(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("sqr", args, 1)
    return args[0] ** 2


def _builtin_sign(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("sign", args, 1)
    return np.sign(np.real(args[0]))


def _builtin_uramp(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("uramp", args, 1)
    return np.maximum(np.real(args[0]), 0.0)


def _builtin_round(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("round", args, 1)
    return np.round(args[0])


def _builtin_floor(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("floor", args, 1)
    return np.floor(args[0])


def _builtin_ceil(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("ceil", args, 1)
    return np.ceil(args[0])


def _builtin_int(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("int", args, 1)
    return np.trunc(args[0])


def _builtin_pow(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("pow", args, 2)
    return args[0] ** args[1]


def _builtin_pwr(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("pwr", args, 2)
    return np.abs(args[0]) ** np.real(args[1])


def _builtin_pwrs(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("pwrs", args, 2)
    return np.sign(np.real(args[0])) * np.abs(args[0]) ** np.real(args[1])


def _builtin_min(args: list[QspiceValue]) -> QspiceValue:
    _expect_min_arity("min", args, 1)
    return _reduce_min(args)


def _builtin_max(args: list[QspiceValue]) -> QspiceValue:
    _expect_min_arity("max", args, 1)
    return _reduce_max(args)


def _builtin_limit(args: list[QspiceValue]) -> QspiceValue:
    _expect_arity("limit", args, 3)
    return np.clip(args[0], args[1], args[2])


BUILTIN_FUNCTIONS: dict[str, BuiltinCallable] = {
    "abs": _builtin_abs,
    "acos": _builtin_acos,
    "angle": _builtin_angle,
    "arccos": _builtin_acos,
    "arcsin": _builtin_asin,
    "arctan": _builtin_atan,
    "asin": _builtin_asin,
    "atan": _builtin_atan,
    "atan2": _builtin_atan2,
    "ceil": _builtin_ceil,
    "conj": _builtin_conj,
    "cos": _builtin_cos,
    "cosh": _builtin_cosh,
    "db": _builtin_db,
    "dB": _builtin_db,
    "exp": _builtin_exp,
    "floor": _builtin_floor,
    "imag": _builtin_imag,
    "int": _builtin_int,
    "limit": _builtin_limit,
    "ln": _builtin_ln,
    "log": _builtin_log,
    "log10": _builtin_log10,
    "mag": _builtin_mag,
    "max": _builtin_max,
    "min": _builtin_min,
    "ph": _builtin_angle,
    "phase": _builtin_angle,
    "pow": _builtin_pow,
    "pwr": _builtin_pwr,
    "pwrs": _builtin_pwrs,
    "real": _builtin_real,
    "round": _builtin_round,
    "sgn": _builtin_sign,
    "sign": _builtin_sign,
    "sin": _builtin_sin,
    "sinh": _builtin_sinh,
    "sqr": _builtin_sqr,
    "sqrt": _builtin_sqrt,
    "tan": _builtin_tan,
    "tanh": _builtin_tanh,
    "uramp": _builtin_uramp,
}

BUILTIN_CONSTANTS: dict[str, QspiceValue] = {
    "e": np.asarray(np.e),
    "mho": np.asarray(1.0),
    "pi": np.asarray(np.pi),
    "s": np.asarray(1.0),
    "t": np.asarray(1e12),
    "g": np.asarray(1e9),
    "meg": np.asarray(1e6),
    "k": np.asarray(1e3),
    "m": np.asarray(1e-3),
    "u": np.asarray(1e-6),
    "n": np.asarray(1e-9),
    "p": np.asarray(1e-12),
    "f": np.asarray(1e-15),
}
