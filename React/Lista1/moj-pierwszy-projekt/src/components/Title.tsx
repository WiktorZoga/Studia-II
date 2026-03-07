interface TitleProps {
    name: string;
    role: string;
}

export const Title = ({ name, role } : TitleProps) => {
    return (
        <div>
            <h2>{name}</h2>
            <p style={{ color: '#000000ff'}}>{role}</p>
        </div>
    );
};