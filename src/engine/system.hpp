export module jolly.system;
import core.types;
import core.thread;
import core.memory;

export namespace jolly {
	struct system {
		virtual ~system() = default;

		virtual void init() = 0;
		virtual void term() = 0;

		virtual void step(f32 ms) = 0;
	};

	struct system_thread : public system {
		system_thread() = default;
		virtual ~system_thread() = default;

		virtual void init() {
			auto start = [](ref<core::thread> _, core::mem<void>&& args) {
				auto sys = args.cast<system_thread>();
				sys->run();
				sys = nullptr;
				return 0;
			};

			thread = core::thread((core::pfn_thread)start, core::mem<void>((void*)this));
		}

		virtual void term() {
			thread.join();
		}

		void yield() {
			thread.yield();
		}

		void sleep(int ms) {
			thread.sleep(ms);
		}

		virtual void run() = 0;

		core::thread thread;
	};
}
