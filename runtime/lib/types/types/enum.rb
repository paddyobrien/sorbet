# frozen_string_literal: true
# typed: true

module T::Types
  # validates that the provided value is within a given set/enum
  class Enum < Base
    attr_reader :values

    def initialize(values)
      @values = values
    end

    # @override Base
    def valid?(obj)
      @values.member?(obj)
    end

    # @override Base
    private def subtype_of_single?(other)
      case other
      when Enum
        (other.values - @values).empty?
      else
        false
      end
    end

    # @override Base
    def name
      "T.enum([#{@values.map(&:inspect).join(', ')}])"
    end

    # @override Base
    def describe_obj(obj)
      obj.inspect
    end
  end
end
