# VolatileMap

`VolatileMap` is a Ruby C extension that provides a `Hash`-like container whose
entries automatically expire. Each instance has a TTL (time-to-live, in
seconds); entries older than the TTL are evicted on the next garbage-collection
cycle, and also lazily on read access. Reading or writing an entry refreshes
its timestamp, so frequently-touched keys stay alive.

## Installation

Add to your Gemfile:

```ruby
gem "volatile_map"
```

Or install directly:

```bash
gem install volatile_map
```

## Usage

```ruby
require "volatile_map"

# TTL is in seconds (Float or Integer)
cache = VolatileMap.new(0.5)

cache["session"] = { user_id: 42 }
cache["session"]            # => { user_id: 42 }
cache.key?("session")       # => true
cache.size                  # => 1

sleep 1
GC.start                    # sweep happens just after GC

cache.size                  # => 0
cache["session"]            # => nil
```

### API

- `VolatileMap.new(ttl)` — construct a map with the given TTL in seconds.
- `map[key] = value` — store a value; resets the entry's timestamp.
- `map[key]` — retrieve a value; returns `nil` if missing or expired.
  Refreshes the timestamp on a hit.
- `map.delete(key)` — remove a key; returns the prior value or `nil`.
- `map.key?(key)` — `true` if the key exists and has not expired.
  Also evicts and refreshes like `[]`.
- `map.size` — number of entries currently stored.
- `map.ttl` — the configured TTL in seconds.

### Semantics

- TTL is measured against a monotonic clock (`CLOCK_MONOTONIC`), so wall-clock
  jumps do not affect expiry.
- Eviction happens at two points:
  1. After every GC cycle (via a postponed job that scans the internal hash).
  2. Lazily on read (`[]`, `key?`) so entries cannot outlive their TTL even if
     no GC has run since they expired.
- Writes (`[]=`) and successful reads (`[]`, `key?` returning true) refresh the
  entry's timestamp.

### Caveats

- The map strongly retains its keys and values until they expire. It is not a
  weak-reference container.
- Not Ractor-shareable in this release.
- Not thread-safe; wrap with your own mutex if you mutate from multiple threads.

## Development

Run `bin/setup` to install dependencies, then `rake compile` to build the
extension and `rake spec` to run the tests.

## License

The gem is available as open source under the terms of the
[MIT License](https://opensource.org/licenses/MIT).
