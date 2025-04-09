#include <iostream>
#include <atomic>
#include <thread>
#include <vector>
#include <chrono>

struct Node
{
	int data;
	std::atomic<Node*> next;

	Node (int inData)
	{
		data = inData;
		next = nullptr;
	}
};

struct Stack
{
    std::atomic<std::pair<Node*, int>> top;

    Stack() : top(std::make_pair(nullptr, 0)) {}

	void push(Node* newNode)
	{
		std::pair<Node*, int> old;
		do
		{
			old = top.load(std::memory_order_relaxed);
			newNode->next.store(old.first, std::memory_order_relaxed);
		} while (!top.compare_exchange_strong
			(old, std::make_pair(newNode, old.second), std::memory_order_release, std::memory_order_relaxed));
	}

	Node* pop()
	{
		std::pair<Node*, int> old;
		Node* new_;
		do
		{
			old = top.load(std::memory_order_relaxed);
			if (old.first == nullptr)
			{
				return nullptr;
			}
			new_ = old.first->next.load(std::memory_order_relaxed);
		} while (!top.compare_exchange_strong
			(old, std::make_pair(new_, old.second + 1), std::memory_order_release, std::memory_order_relaxed));

		return old.first;
	}

};

int main() {
    Stack stack;

    Node* nodeA = new Node(100);
    stack.push(nodeA);
    std::cout << "Initial push: node " << nodeA->data << "\n";

    std::thread t1([&]() {
        std::pair<Node*, int> oldTop = stack.top.load();
        std::cout << "Thread 1 sees: " << oldTop.first->data
            << " (counter: " << oldTop.second << ")\n";

        // Delay to allow ABA operations to occur in another thread.
        std::this_thread::sleep_for(std::chrono::milliseconds(500));

        // Thread 1 now attempts a compare_exchange using its snapshot.
        // It will try to set the top to nullptr (simulating an update).
        std::pair<Node*, int> expected = oldTop;
        bool success = stack.top.compare_exchange_strong(
            expected, std::make_pair(nullptr, oldTop.second + 1),
            std::memory_order_release, std::memory_order_relaxed);

        // If ABA occurred, the pointer might still be nodeA but the counter is different,
        // making this CAS fail.
        std::cout << "Thread 1: CAS with snapshot "
            << (success ? "succeeded" : "failed")
            << " (expected counter: " << oldTop.second << ", current counter: "
            << stack.top.load().second << ")\n";
        });

    std::thread t2([&]() {
        std::this_thread::sleep_for(std::chrono::milliseconds(100));

        // Perform ABA: pop nodeA then push it back.
        Node* popped = stack.pop();
        if (popped) {
            std::cout << "Thread 2: Popped node " << popped->data << "\n";
        }

        // Immediately push the popped node back.
        stack.push(popped);
        std::cout << "Thread 2: Pushed node " << popped->data << "\n";
        });

    t1.join();
    t2.join();
    Node* remaining = stack.pop();
    if (remaining) {
        delete remaining;
    }
    return 0;
}
