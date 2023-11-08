from abc import abstractmethod, ABCMeta
from typeWrappers.types import DbType
from datetime import datetime, timedelta
from typing import Generator
from enum import IntFlag
import random
import string
import os


class GenerationMode(IntFlag):
    RANDOM = 0x01
    INCREASING = 0x02
    DECREASING = 0x04
    REPEATING = 0x08
    NAMESURNAME = 0x10
    EMAIL = 0x20
    PHONE = 0x40
    NATURALTEXT = 0x80

def preload_data(file_name: str) -> list[str]:
    with open(os.path.join('data', file_name), 'r') as file:
        return [line.strip() for line in file.readlines()]

def purge_quotes(string: str) -> str:
    return string.replace("'", "")

FEMALE_NAMES = [n.lower() for n in preload_data('female-names-list.txt')]
MALE_NAMES = [n.lower() for n in preload_data('male-names-list.txt')]
SURNAMES = [n.lower() for n in preload_data('surnames-list.txt')]
NATURAL_ENGLISH_WORDS = [purge_quotes(w) for w in preload_data('words.txt')]
KNOWN_EMAIL_DOMAINS = domains = [
  "gmail.com",
  "yahoo.com",
  "hotmail.com",
  "aol.com",
  "hotmail.co.uk",
  "hotmail.fr",
  "msn.com",
  "yahoo.fr",
  "wanadoo.fr",
  "orange.fr",
  "comcast.net",
  "yahoo.co.uk",
  "yahoo.com.br",
  "yahoo.co.in",
  "live.com",
  "rediffmail.com",
  "free.fr",
  "gmx.de",
  "web.de",
  "yandex.ru",
  "ymail.com",
  "libero.it",
  "outlook.com",
  "uol.com.br",
  "bol.com.br",
  "mail.ru",
  "cox.net",
  "hotmail.it",
  "sbcglobal.net",
  "sfr.fr",
  "live.fr",
  "verizon.net",
  "live.co.uk",
  "googlemail.com",
  "yahoo.es",
  "ig.com.br",
  "live.nl",
  "bigpond.com",
  "terra.com.br",
  "yahoo.it",
  "neuf.fr",
  "yahoo.de",
  "alice.it",
  "rocketmail.com",
  "att.net",
  "laposte.net",
  "facebook.com",
  "bellsouth.net",
  "yahoo.in",
  "hotmail.es",
  "charter.net",
  "yahoo.ca",
  "yahoo.com.au",
  "rambler.ru",
  "hotmail.de",
  "tiscali.it",
  "shaw.ca",
  "yahoo.co.jp",
  "sky.com",
  "earthlink.net",
  "optonline.net",
  "freenet.de",
  "t-online.de",
  "aliceadsl.fr",
  "virgilio.it",
  "home.nl",
  "qq.com",
  "telenet.be",
  "me.com",
  "yahoo.com.ar",
  "tiscali.co.uk",
  "yahoo.com.mx",
  "voila.fr",
  "gmx.net",
  "mail.com",
  "planet.nl",
  "tin.it",
  "live.it",
  "ntlworld.com",
  "arcor.de",
  "yahoo.co.id",
  "frontiernet.net",
  "hetnet.nl",
  "live.com.au",
  "yahoo.com.sg",
  "zonnet.nl",
  "club-internet.fr",
  "juno.com",
  "optusnet.com.au",
  "blueyonder.co.uk",
  "bluewin.ch",
  "skynet.be",
  "sympatico.ca",
  "windstream.net",
  "mac.com",
  "centurytel.net",
  "chello.nl",
  "live.ca",
  "aim.com",
  "bigpond.net.au",
]

VALID_PATTERNS_PER_TYPE: dict[DbType, GenerationMode] = {
    DbType.INTEGER: GenerationMode.RANDOM
    | GenerationMode.INCREASING
    | GenerationMode.DECREASING
    | GenerationMode.REPEATING,
    DbType.STRING: GenerationMode.RANDOM
    | GenerationMode.REPEATING
    | GenerationMode.NAMESURNAME
    | GenerationMode.EMAIL
    | GenerationMode.PHONE
    | GenerationMode.NATURALTEXT,
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
                case GenerationMode.NAMESURNAME:
                    NAME_LIST = random.choice([MALE_NAMES, FEMALE_NAMES])
                    random_name = random.choice(NAME_LIST).title()
                    random_surname = random.choice(SURNAMES).title()
                    random_str = f"{random_name} {random_surname}"
                    yield random_str
                case GenerationMode.EMAIL:
                    # name.surname@domain.tld
                    NAME_LIST = random.choice([MALE_NAMES, FEMALE_NAMES])
                    random_name = random.choice(NAME_LIST)
                    random_surname = random.choice(SURNAMES)
                    random_domain = random.choice(KNOWN_EMAIL_DOMAINS)
                    random_str = f"{random_name}.{random_surname}@{random_domain}"
                    yield random_str
                case GenerationMode.PHONE:
                    PHONE_LENGTH = 10
                    random_str = "".join(
                        random.choices(string.digits, k=PHONE_LENGTH)
                    )
                    yield random_str
                case GenerationMode.NATURALTEXT:
                    random_str = " ".join(
                        random.choices(NATURAL_ENGLISH_WORDS, k=pattern_number)
                    )
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
