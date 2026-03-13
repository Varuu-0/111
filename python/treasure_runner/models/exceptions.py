from treasure_runner.bindings import Status

class GameError(Exception):
    """Base class for all game exceptions."""

class GameEngineError(GameError):
    """Base class for logic errors in the game engine."""

class InvalidArgumentError(GameEngineError):
    """Exception for invalid arguments."""

class OutOfBoundsError(GameEngineError):
    """Exception for out of bounds access."""

class ImpassableError(GameEngineError):
    """Exception for impassable terrain."""

class NoSuchRoomError(GameEngineError):
    """Exception when a room does not exist."""

class NoPortalError(GameEngineError):
    """Exception for missing portals."""

class InternalError(GameEngineError):
    """Exception for internal logic errors."""

class StatusError(GameError):
    """Base class for status code errors."""

class StatusInvalidArgumentError(StatusError):
    """Status error for invalid arguments."""

class StatusNullPointerError(StatusError):
    """Status error for null pointers."""

class StatusNoMemoryError(StatusError):
    """Status error for out of memory."""

class StatusBoundsExceededError(StatusError):
    """Status error for out of bounds."""

class StatusNotFoundError(StatusError):
    """Status error for missing items."""

class StatusDuplicateError(StatusError):
    """Status error for duplicates."""

class StatusInvalidStateError(StatusError):
    """Status error for invalid states."""

class StatusImpassableError(StatusError):
    """Status error for impassable terrain."""

class StatusInternalError(StatusError):
    """Status error for internal errors."""

def status_to_status_exception(raw_status, message: str = "") -> None:
    """Maps Status enum to the appropriate exception."""
    try:
        status = Status(raw_status)
    except ValueError as exc:
        raise GameEngineError(f"Unknown Status Code ({raw_status}): {message}") from exc

    if status == Status.OK:
        return

    mapping = {
        Status.INVALID_ARGUMENT: StatusInvalidArgumentError,
        Status.NULL_POINTER: StatusNullPointerError,
        Status.NO_MEMORY: StatusNoMemoryError,
        Status.BOUNDS_EXCEEDED: StatusBoundsExceededError,
        Status.INTERNAL_ERROR: StatusInternalError,
        Status.ROOM_IMPASSABLE: StatusImpassableError,
        Status.ROOM_NO_PORTAL: NoPortalError,
        Status.ROOM_NOT_FOUND: StatusNotFoundError,
        Status.GE_NO_SUCH_ROOM: NoSuchRoomError,
        Status.WL_ERR_CONFIG: StatusInternalError,
        Status.WL_ERR_DATAGEN: StatusInternalError,
    }

    exc_class = mapping.get(status, GameEngineError)
    raise exc_class(f"Status Code ({status.name}): {message}")

def status_to_exception(raw_status, message: str = "") -> None:
    """Maps Status enum to the appropriate high-level exception."""
    try:
        status = Status(raw_status)
    except ValueError as exc:
        raise GameEngineError(f"Unknown Status Code ({raw_status}): {message}") from exc

    if status == Status.OK:
        return

    mapping = {
        Status.INVALID_ARGUMENT: InvalidArgumentError,
        Status.NULL_POINTER: InternalError,
        Status.NO_MEMORY: InternalError,
        Status.BOUNDS_EXCEEDED: OutOfBoundsError,
        Status.INTERNAL_ERROR: InternalError,
        Status.ROOM_IMPASSABLE: ImpassableError,
        Status.ROOM_NO_PORTAL: NoPortalError,
        Status.ROOM_NOT_FOUND: NoSuchRoomError,
        Status.GE_NO_SUCH_ROOM: NoSuchRoomError,
        Status.WL_ERR_CONFIG: InternalError,
        Status.WL_ERR_DATAGEN: InternalError,
    }

    exc_class = mapping.get(status, GameEngineError)
    raise exc_class(f"Status Code ({status.name}): {message}")
    