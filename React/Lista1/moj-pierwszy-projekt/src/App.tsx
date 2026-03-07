import { BussinessCard } from "./components/BusinessCard";

function App() {
	const myData = {
		name: "Wiktor Zoga",
		role: "Algorithm Engineer",
		avatarUrl: 'https://www.sfzoo.org/wp-content/uploads/2025/07/Red-panda-Little-mebo_NC5_resize-crop.jpg',
		email: "wiktor.zoga@gmail.com",
		phone: "+48 663 992 667",
		website: 'https://github.com/WiktorZoga',
		about: "Driven by a passion for mathematics, I specialize in exploring probability theory and the world of deep learning.",
		skills: [
			'Math', 'Algorithms & Data Structures', 'Machine Learning', 'Multithreading', 'Python', 'C++'
		]
	};

	return (
		<div style={{ backgroundColor: '#f0f2f5', minHeight: '100vh', padding: '20px' }}>
			<BussinessCard data={myData} />
		</div>
	);
};

export default App;