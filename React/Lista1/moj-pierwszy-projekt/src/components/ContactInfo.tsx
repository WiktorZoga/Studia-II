import { ContactItem } from './ContactItem';

interface ContactInfoProps {
    email: string;
    phone: string;
    website: string;
}

export const ContactInfo = ({ email, phone, website } : ContactInfoProps) => {
    return (
        <div>
            <h3>Contact Information</h3>
            <ContactItem label="Email" value={email} />
            <ContactItem label="Phone" value={phone} />
            <ContactItem label="Website" value={website} />
        </div>
    );
};