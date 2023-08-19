# frozen_string_literal: true

source "https://rubygems.org"

git_source(:github) {|repo_name| "https://github.com/#{repo_name}" }

# Latest cucumber 8.0.0 implies cucumber-wire 0.0.1, which seems wrong, and fails `require` in
# support/wire.rb. At time of writing, pinning cucumber-wire to 6.x implies cucumber 7.1.
gem "cucumber-wire", "> 6"
gem "cucumber"