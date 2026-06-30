# frozen_string_literal: true

require 'objspace'

RSpec.describe VolatileMap do
  let(:map) { VolatileMap.new(5) }

  before do
    map[:foo] = :bar
  end

  it "has a version number" do
    expect(VolatileMap::VERSION).not_to be nil
  end

  describe "#tll" do
    subject { map.ttl }

    it { is_expected.to eq(5.0) }
  end

  describe "#size" do
    subject { map.size }

    it { is_expected.to eq(1) }
  end

  describe "#length" do
    subject { map.length }

    it { is_expected.to eq(1) }
  end

  describe "#delete" do
    subject(:delete) { map.delete(key) }

    context "when the given key does not exist" do
      let(:key) { :bar }

      it { is_expected.to be_nil }
    end

    context "when the given key exists" do
      let(:key) { :foo }

      it "deletes the key" do
        expect { delete }.to change { map[:foo] }.from(:bar).to(nil)
      end

      it "returns value of the key" do
        expect(delete).to eq(:bar)
      end
    end
  end

  describe "key check methods" do
    shared_examples_for "a key check method" do
      context "when the given key does not exist" do
        let(:key) { :bar }

        it { is_expected.to be(false) }
      end

      context "when the given key exists" do
        let(:key) { :foo }

        it { is_expected.to be(true) }
      end
    end

    it_behaves_like "a key check method" do
      subject { map.key?(key) }
    end

    it_behaves_like "a key check method" do
      subject { map.has_key?(key) }
    end

    it_behaves_like "a key check method" do
      subject { map.include?(key) }
    end

    it_behaves_like "a key check method" do
      subject { map.member?(key) }
    end
  end

  describe "#[]" do
    subject(:value) { map[key] }

    context "when the given key does not exist" do
      let(:key) { :bar }

      it { is_expected.to be_nil }
    end

    context "when the given key exists" do
      let(:key) { :foo }

      before do
        map[key] = :bar
      end

      context "when the given key is expired" do
        before do
          VolatileMap.set_test_clock!(6)
        end

        it "returns nil" do
          expect(value).to be_nil
        end
      end

      context "when the given key is not expired" do
        before do
          VolatileMap.set_test_clock!(5)
        end

        it "returns the value" do
          expect(value).to eq(:bar)
        end

        context "accessing the same value multiple times" do
          it "refreshes the TTL of the key" do
            expect(map[key]).to eq(:bar)

            VolatileMap.set_test_clock!(10)

            expect(map[key]).to eq(:bar)
          end
        end
      end
    end
  end

  describe "#[]=" do
    context "when the value is not set yet" do
      it "sets the value" do
        expect { map[:bar] = :foo }.to change { map[:bar] }.from(nil).to(:foo)
      end
    end

    context "when the value is already set" do
      it "sets the value" do
        expect { map[:foo] = :zoo }.to change { map[:foo] }.from(:bar).to(:zoo)
      end

      it "refreshes the TTL" do
        VolatileMap.set_test_clock!(5)

        map[:foo] = :zoo

        VolatileMap.set_test_clock!(10)

        expect(map[:foo]).to eq(:zoo)
      end
    end
  end

  describe "key volatility" do
    it "removes the key from the map" do
      VolatileMap.set_test_clock!(6)

      GC.start # This will register the postponed job
      Thread.pass # This will give the VM a chance to run it

      expect(map.size).to eq(0)
      expect(map[:foo]).to be_nil
    end
  end
end
