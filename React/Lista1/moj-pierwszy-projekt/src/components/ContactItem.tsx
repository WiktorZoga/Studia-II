interface ContactItemProps {
    label: string;
    value: string;
}

export const ContactItem = ({ label, value } : ContactItemProps) => {
    return (
        <div style={{ margin: '8px 0' }}>
            <strong>{label}: </strong>
            <span>{value}</span>
        </div>
    );
};