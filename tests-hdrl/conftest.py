import uuid

import pytest


@pytest.fixture
def make_tmp_filename(tmp_path):
    """
    Temporary filename factory based on random uuids. The returned creator provides
    an absolute path to the temporary file as a string. The temoprary file is deleted
    automatically when the fixture is torn down.
    """
    filepath = None

    def _make_tmp_filename():
        filepath = tmp_path.joinpath(str(uuid.uuid4()) + ".fits")
        return str(filepath.resolve())

    yield _make_tmp_filename
    if filepath is not None:
        filepath.unlink(missing_ok=True)


@pytest.fixture(autouse=True)
def reset_hdrl_state():
    """
    Reset HDRL library state between tests to prevent state pollution.
    This fixture runs automatically for all tests.
    """
    # Import here to avoid circular imports
    import hdrl.core
    import cpl.core as cplcore
    
    # Force garbage collection to clean up any remaining HDRL objects
    import gc
    gc.collect()
    
    # Try to explicitly clean up any remaining CPL/HDRL objects
    # This might help with memory management issues
    try:
        # Force cleanup of any remaining image objects
        gc.collect()
    except Exception:
        pass
    
    yield
    
    # Clean up after the test as well
    gc.collect()
    
    # Additional cleanup after test
    try:
        gc.collect()
    except Exception:
        pass
