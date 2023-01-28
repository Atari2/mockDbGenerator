from abc import abstractmethod, ABCMeta
from typeWrappers.types import DbType
from datetime import datetime, timedelta
from typing import Generator
from enum import IntFlag
import random
import string


class GenerationMode(IntFlag):
    RANDOM = 0x01
    INCREASING = 0x02
    DECREASING = 0x04
    REPEATING = 0x08


VALID_PATTERNS_PER_TYPE: dict[DbType, GenerationMode] = {
    DbType.INTEGER: GenerationMode.RANDOM
    | GenerationMode.INCREASING
    | GenerationMode.DECREASING
    | GenerationMode.REPEATING,
    DbType.STRING: GenerationMode.RANDOM | GenerationMode.REPEATING,
    DbType.REAL: GenerationMode.RANDOM
    | GenerationMode.INCREASING
    | GenerationMode.DECREASING
    | GenerationMode.REPEATING,
    DbType.DATE: GenerationMode.RANDOM
    | GenerationMode.INCREASING
    | GenerationMode.DECREASING,
}

_T_TYPES = str | int | float | bool | datetime
_PATT_TYPES = str | int | float | bool | timedelta | None


class ValueGenerator(metaclass=ABCMeta):
    quantity: int
    generation_mode: GenerationMode

    def __init__(self, quantity: int, generation_mode: GenerationMode):
        self.quantity = quantity
        self.generation_mode = generation_mode

    @abstractmethod
    def generate(self, pattern_number: _PATT_TYPES) -> Generator[_T_TYPES, None, None]:
        raise NotImplementedError


class GenerateInteger(ValueGenerator):
    current_value: int
    generated_values: int

    def __init__(
        self, quantity: int, generation_mode: GenerationMode, starting_value: int = 0
    ):
        self.generated_values = 0
        self.current_value = starting_value
        super().__init__(quantity, generation_mode)

    def generate(self, pattern_number: int) -> Generator[int, None, None]:
        while self.generated_values < self.quantity:
            self.generated_values += 1
            match self.generation_mode:
                case GenerationMode.RANDOM:
                    yield random.randint(0, pattern_number)
                case GenerationMode.INCREASING:
                    value = self.current_value
                    self.current_value += pattern_number
                    yield value
                case GenerationMode.DECREASING:
                    value = self.current_value
                    self.current_value -= pattern_number
                    yield value
                case GenerationMode.REPEATING:
                    value = self.current_value
                    if self.generated_values % pattern_number == 0:
                        self.current_value = 0
                    else:
                        self.current_value += 1
                    yield value
        return


class GenerateString(ValueGenerator):
    repeated_values: list[str]
    generated_values: int

    def __init__(self, quantity: int, generation_mode: GenerationMode):
        self.repeated_values = []
        self.generated_values = 0
        super().__init__(quantity, generation_mode)

    def generate(self, pattern_number: int) -> Generator[str, None, None]:
        while self.generated_values < self.quantity:
            self.generated_values += 1
            match self.generation_mode:
                case GenerationMode.RANDOM:
                    yield "".join(
                        random.choices(string.ascii_letters, k=pattern_number)
                    )
                case GenerationMode.REPEATING:
                    if len(self.repeated_values) == pattern_number:
                        yield self.repeated_values[
                            self.generated_values % pattern_number
                        ]
                    else:
                        random_str = "".join(
                            random.choices(string.ascii_letters, k=pattern_number)
                        )
                        self.repeated_values.append(random_str)
                        yield random_str
        return


class GenerateReal(ValueGenerator):
    current_value: float
    generated_values: int

    def __init__(
        self,
        quantity: int,
        generation_mode: GenerationMode,
        starting_value: float = 0.0,
    ):
        self.generated_values = 0
        self.current_value = starting_value
        super().__init__(quantity, generation_mode)

    def generate(self, pattern_number: float) -> Generator[float, None, None]:
        while self.generated_values < self.quantity:
            self.generated_values += 1
            match self.generation_mode:
                case GenerationMode.RANDOM:
                    yield random.random() * pattern_number
                case GenerationMode.INCREASING:
                    value = self.current_value
                    self.current_value += pattern_number
                    yield value
                case GenerationMode.DECREASING:
                    value = self.current_value
                    self.current_value -= pattern_number
                    yield value
                case GenerationMode.REPEATING:
                    value = self.current_value
                    if self.generated_values % pattern_number == 0:
                        self.current_value = 0.0
                    else:
                        self.current_value += 1.0
                    yield value
        return


class GenerateDate(ValueGenerator):
    current_value: datetime
    generated_values: int

    def __init__(
        self,
        quantity: int,
        generation_mode: GenerationMode,
        starting_value: datetime = datetime.fromtimestamp(0),
    ):
        self.generated_values = 0
        self.current_value = starting_value
        super().__init__(quantity, generation_mode)

    def generate(self, pattern_number: timedelta) -> Generator[datetime, None, None]:
        if not isinstance(pattern_number, timedelta):
            raise TypeError(f"Expected timedelta, got {type(pattern_number)}")
        while self.generated_values < self.quantity:
            self.generated_values += 1
            match self.generation_mode:
                case GenerationMode.RANDOM:
                    yield datetime.fromtimestamp(random.randint(0, 2147483647))
                case GenerationMode.INCREASING:
                    value = self.current_value
                    self.current_value += pattern_number
                    yield value
                case GenerationMode.DECREASING:
                    value = self.current_value
                    self.current_value -= pattern_number
                    yield value
        return


VALUE_GENERATORS = {
    DbType.INTEGER: GenerateInteger,
    DbType.STRING: GenerateString,
    DbType.REAL: GenerateReal,
    DbType.DATE: GenerateDate,
}


def value_generator_factory(
    db_type: DbType,
    quantity: int,
    generation_mode: GenerationMode = GenerationMode.RANDOM,
    starting_value: _T_TYPES | None = None,
) -> ValueGenerator:
    if valid_modes := VALID_PATTERNS_PER_TYPE[db_type]:
        if not valid_modes & generation_mode:
            raise ValueError(f"Invalid generation mode for type {db_type.name}")
    if starting_value is not None:
        if db_type == DbType.STRING:
            raise ValueError("Cannot specify starting value for boolean or string type")
        return VALUE_GENERATORS[db_type](quantity, generation_mode, starting_value)
    return VALUE_GENERATORS[db_type](quantity, generation_mode)
